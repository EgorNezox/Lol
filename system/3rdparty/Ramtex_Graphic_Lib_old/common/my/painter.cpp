#include "painter.h"

#define  ADD(xy1,xy2) (((GXYT)(xy1))+((GXYT)(xy2)))
#define  SUB(xy1,xy2) (((GXYT)(xy1))-((GXYT)(xy2)))
#define  MIRX( e1, e2) ((mx!= 0) ? SUB(e1,e2) : ADD(e1,e2))
#define  MIRY( e1, e2) ((my!= 0) ? SUB(e1,e2) : ADD(e1,e2))
#define  SWPX( e1, e2) ((mx!= 0) ? (e1) : (e2))

GPainter::GPainter(Display *aDisplay)
{
    display = aDisplay;
    uint16_t height =  display->getHeight();
    uint16_t width =  display->getWidth();

    tabSize = width / 6;

    viewports = new GViewPort[viewPortCount];
    tmpbuf = new GColor[width];

    for (uint8_t i = 0; i < viewPortCount; i++ )
    {
        curViewPort = &viewports[i];
        resetVP(0);
    }

    viewPortNum = 0;
    curViewPort = &viewports[0];

    curViewPort->bgColor = 0x0000;
    curViewPort->frColor = 0xFFFF;

    display->setViewPort({0,0,width-1,height-1});
    display->update();


//    buffer = new GColor* [height];
//    for (int count = 0; count < width; count++)
//      buffer[count] = new GColor [height];
}

GPoint GPainter::percentToPixel(GPointF gPointF)
{
	uint16_t dx = curViewPort->end.x - curViewPort->begin.x;
	uint16_t xPer = (gPointF.x * dx) / 100;
	uint16_t dy = curViewPort->end.y - curViewPort->begin.y;
	uint16_t yPer = (gPointF.y * dy) / 100;
	return {xPer,yPer};
}

uint16_t GPainter::gfGetFontH( GFont* fp )
{
    if (fp == NULL)
        return SYSFONT.symHeight;  /* default height */
    return getSymH(fp);
}

uint16_t GPainter::gGetFontH()
{
    //datacheck();    /* check internal data for errors */
    return gfGetFontH( curViewPort->font );
}

uint16_t GPainter::gGetFontHvp(uint8_t viewPortNum)
{
    //GCHECKVP(vp);
    return gfGetFontH(viewports[viewPortNum].font);
}

void GPainter::resetVP(uint8_t resettype)
{
   setBx(0);
   setBy(0);
   setEx(display->getWidth() - 1);
   setEy(display->getHeight() - 1);

   setPixPosX(0);
   setPixPosY(0);

   setSymPosX(0);
   if (resettype <= 1)
   {
       /*gcurvp->cpos.y = SYSFONT.symheight-1;*/
       setSymPosY(8 - 1); /* default for SYSFONT */
       setMode(GNORMAL);

       setFont((GFont*)&SYSFONT);
       setCodePage(SYSFONT.codePage);

       if (resettype == 0)
       {
          // setFrColor(display->getFrColor());
          // setBgColor(display->getBgColor());
       }
   }
   else
   {
      setSymPosY(getSymH(getFont()) - 1);
   }
   calcDataCheck();
}

void GPainter::setColor(GColor bgColor, GColor fgColor)
{
    setBgColor(bgColor);
    setFrColor(fgColor);
}

void GPainter::limitToVP(uint16_t* xs, uint16_t* ys, uint16_t* xe, uint16_t* ye)
{
    uint16_t xsT = *xs;
    uint16_t ysT = *xs;
    uint16_t xeT = *xs;
    uint16_t yeT = *xs;

    if ( xsT < getBx() ) xsT = getBx();
    if ( xsT > getEx() ) xsT = getEx();
    if ( xeT > getEx() ) xeT = getEx();
    if ( xeT < xsT     ) xeT = xsT;

    if ( ysT < getBy() ) ysT = getBy();
    if ( ysT > getEy() ) ysT = getEy();
    if ( yeT > getEy() ) yeT = getEy();
    if ( yeT < ysT     ) yeT = ysT;

    *xs = xsT;
    *ys = ysT;
    *xe = xeT;
    *ye = yeT;
}

void GPainter::limitToViewPort(uint16_t* xs, uint16_t* ys, uint16_t* xe, uint16_t* ye)
{
    uint16_t xsT = *xs;
    uint16_t ysT = *ys;
    uint16_t xeT = *xe;
    uint16_t yeT = *ye;

    if ( xsT < getBx() ) xsT = getBx();
    if ( xeT > getEx() ) xeT = getEx();
    if ( ysT < getBy() ) ysT = getBy();
    if ( yeT > getEy() ) yeT = getEy();

    if( xsT > xeT ) // swap
    {
        uint16_t t;
        t = xsT;
        xsT = xeT;
        xeT = t;
    }
    if( ysT > yeT ) // swap
    {
        uint16_t t;
        t = ysT;
        ysT = yeT;
        yeT = t;
    }

    *xs = xsT;
    *ys = ysT;
    *xe = xeT;
    *ye = yeT;
}

void GPainter::setViewPort()
{
	GPoint begin {0,0};
	GPoint end {display->getWidth() - 1, display->getHeight() - 1};
	setViewPort(begin,end);
}

void GPainter::setViewPort(GPointF begin, GPointF end)
{
	setViewPort(percentToPixel(begin),percentToPixel(end));
}

void GPainter::setViewPort(GPoint begin, GPoint end )
{
    uint16_t w = display->getWidth() - 1;
    uint16_t h = display->getHeight() - 1;

    if ( begin.x > w     ) begin.x = w;
    if ( end.x   > w     )   end.x = w;
    if ( begin.y > h     ) begin.y = h;
    if ( end.y   > h     )   end.y = h;
    if ( begin.x > end.x ) begin.x = end.x;
    if ( begin.y > end.y ) begin.y = end.y;

    //  gi_datacheck(); /* check internal data for errors */

    setBx(begin.x);
    setBy(begin.y);
    setEx(end.x);
    setEy(end.y);

    setSymPosX(begin.x);
    uint16_t posY = begin.y + ((curViewPort->font != NULL) ? getSymH(curViewPort->font) - 1 : 8 - 1);
    setSymPosY(posY);
    GLIMITU(curViewPort->symbolPos.y, curViewPort->end.y);

    setPixPosX(begin.x);
    setPixPosY(begin.y);

    //calcDataCheck(); /* correct VP to new settings */
}

void GPainter::setPos( GPoint pos )
{
    // gi_datacheck(); /* check internal data for errors */

    pos.x += getBx();
    pos.y += getBy();

    if( pos.x < getBx() ) pos.x = getBx();
    if( pos.x > getEx() ) pos.x = getEx();
    if( pos.y < getBy() ) pos.y = getBy();
    if( pos.y > getEy() ) pos.y = getEy();

    setPixPosX(pos.x);
    setPixPosY(pos.y);

    setSymPosX(pos.x);
    setSymPosY(pos.y);

    //calcDataCheck(); /* correct VP to new settings */
}

void GPainter::moveTo(GPoint point)
{
    //gi_datacheck(); /* check internal data for errors */

    setPixPosX(point.x + getBx());
    setPixPosY(point.y + getBy());

    //calcDataCheck(); /* correct VP to new settings */
}

void GPainter::drawPixel(GPoint point, GColor color)
{
    GLIMITU(point.y, display->getHeight() - 1);
    GLIMITU(point.x, display->getWidth()  - 1);

    display->setViewPort({point.x, point.y, point.x, point.y});
    display->setHautoInc();
    display->autoWrite(color);
}

void GPainter::lineTo(GPoint point)
{
    uint16_t xs; /* Pos. in view-port */
    uint16_t ys;

    uint16_t xe = point.x; /* Pos. in view-port */
    uint16_t ye = point.y;

   // gi_datacheck(); /* check internal data for errors */

    xs += getBx();
    ys += getBy();

    xs = getPixPosX();
    ys = getPixPosY();
    setPixPosX(xe);
    setPixPosY(ye);

   // glcd_err = 0;
    if( (xs == xe) || (ys == ye) )
    {
       limitToViewPort(&xs, &ys, &xe, &ye);
       drawRectangle( xs, ys, xe, ye, getGroundColor());
       //drawRect( {xs, ys}, {xe, ye}, getGroundColor());
    }
    else
    {
        drawLine( {xs, ys}, {xe, ye} );
    }

  //  updatehw();
  //  calcDataCheck(); /* correct VP to new settings */
}

void GPainter::drawLine(GPoint begin, GPoint end)
{
    int16_t wid, deltax1, deltax2, diaginc, loop;
    int16_t hei, deltay1, deltay2, nondiag;
    int16_t d, temp;
    int16_t x1,y1;

    /* Make safe difference calculation and conversion to signed int. */
    x1 = (int16_t) ((uint16_t) begin.x);
    y1 = (int16_t) ((uint16_t) begin.y);

    if (end.x >= begin.x)
       wid = (int16_t)((uint16_t) (end.x - begin.x));
    else
    {
        wid = (int16_t)((uint16_t) (begin.x - end.x));
        wid *= -1;
    }

    if (end.y >= begin.y)
        hei = (int16_t) ((uint16_t) (end.y - begin.y));
    else
    {
        hei = (int16_t) ((uint16_t) (begin.y - end.y));
        hei *= -1;
    }

    if ( wid < 0 )
    {
        wid = -wid;
        deltax1 = -1;
    }
    else
        deltax1 = 1;

    if ( hei < 0 )
    {
        hei = -hei;
        deltay1 = -1;
    }
    else
        deltay1 = 1;

    if ( wid < hei )
    {
        /* swap wid and hei */
        temp = wid;
        wid = hei;
        hei = temp;
        deltax2 = 0;
        deltay2 = deltay1;
    }
    else
    {
        deltax2 = deltax1;
        deltay2 = 0;
    }

    nondiag = hei * 2;
    d = nondiag - wid;
    diaginc = d - wid;

    for ( loop = 0; loop <= wid; loop++ )
    {
        begin.x = (uint16_t) x1;
        begin.y = (uint16_t) y1;
        if( !(begin.x < getBx() ||  /* if outside do not draw */
              begin.x > getEx() ||
              begin.y < getBy() ||
              begin.y > getEy()) )
            drawPixel( {begin.x, begin.y} , getGroundColor() );

        if ( d < 0 )
        {
            x1 += deltax2;
            y1 += deltay2;
            d += nondiag;
        }
        else
        {
            x1 += deltax1;
            y1 += deltay1;
            d += diaginc;
        }
    }
}

/* Внешний API. Нарисовать текст. */

void GPainter::drawText( const char* text )
{
    StringMarks sm;
    sm.next = text;
    drawText( &sm );
}

/* Внутренний API. Нарисовать текст. */

void GPainter::drawText( StringMarks* stringMarks )
   {
   uint8_t ln;
   uint16_t fontHeight;
   uint16_t viewPortHeight;
   bool isTransmode;

   if ( stringMarks->next == NULL )
      return;

   isFullHeightLine = !isPartialLine();
   fontHeight = gfGetFontH( curViewPort->font );
   viewPortHeight = (getEy() - getBy()) + 1;
   isTransmode = (isTransparent() && !isInverse());

   if (isFullHeightLine && (viewPortHeight < fontHeight))
   {
      // G_WARNING( "gputch: Viewport height too small for character" );
       return; /* Viewport too small for font, skip output */
   }

   if (isAlignCenterV() || isAlignBottom())
      {
      /* Preprocess whole string for y size calculation */
      /* (incl any wrap lines or multibyte characters) */
      uint16_t lines;

      uint16_t symPosXtmp;
      const char* cp;

      for (lines = 0, symPosXtmp = getSymPosX(), cp = stringMarks->next ; ; )
      {
          if ((ln = sizeStringSeg(stringMarks)) <= 1)
          {
              if (ln == 1)
                  lines += 1; /* single line, else empty line detected */
              break;
          }
          /* Multiline string detected (optionally caused by character or word wrapping) */
          /* Find total number of lines for prescroll or vertical alignment */
          if (ln == 3)
              lines += 1;  /* \n or line wrap (else \r) */
          setSymPosX(getBx());
      }

      /* Restore start position after size calculation */
      stringMarks->next = cp;
      setSymPosX(symPosXtmp);

      if (isAlignCenterV())
      {
          setSymPosY(fontHeight - 1 + getBy()); /* Overflow, align to top */
          if ((uint16_t)fontHeight * lines < (uint16_t)viewPortHeight)
              setSymPosY(getSymPosY() + (viewPortHeight - (fontHeight * lines)) / 2);            /* Center lines */
      }
      else /* isAlignBottom()*/
      {
          if ((uint16_t)fontHeight * lines > (uint16_t)viewPortHeight)
          {
              /* Some lines are skipped or cut */
              setSymPosY( getBy() + ((viewPortHeight - 1) % fontHeight));       /* Set top (partial) line pos */
              if (isFullHeightLine)
              {
                  lines -= viewPortHeight / fontHeight;        /* partial top line is not shown */
                  setSymPosY(getSymPosY() + fontHeight);
              }
              else
                  lines -= ((uint16_t)viewPortHeight + fontHeight - 1) / fontHeight; /* top line is partial shown */

              /* Skip invisible lines from string in advance */
              while (lines-- > 0)
              {
                  sizeStringSeg(stringMarks);
              }
          }
          else
              setSymPosY(getEy() - fontHeight * (lines - 1));
      }
   }
   else
      if (isAlignTop())
         setSymPosY( getBy() + fontHeight - 1);  /* Move to top, compensate for font offset */

   /* Output symbols */
   /* get line segment (handle all word wrap or word cut calculations) */
   ln = sizeStringSeg(stringMarks);

   /* Clear viewport above string ? */
   if (isVpClrUp() && (getSymPosY() >= fontHeight) && !isTransmode)
      fill( getBx(), getBy(), getEx(), getSymPosY() - fontHeight, (uint16_t)(getGroundColor()) );

    if (ln != 0)
    {
       do
       {
          if (isAlignCenterH())
            setSymPosX( (stringMarks->len >= ((uint16_t)getEx() - getBx() + 1)) ? getBx() : (uint16_t)getBx() + (((uint16_t)getEx() - getBx() + 1) - stringMarks->len) / 2);
          else
            if (isAlignRight())
              setSymPosX( (stringMarks->len >= ((uint16_t)getEx() - getBx() + 1)) ? getBx() : (((uint16_t)getEx() + 1) - stringMarks->len));

          /* Clear viewport to the left of the string segment ? */
          if (isVpClrLeft() && (getSymPosX() > getBx()) && !isTransmode)
                  fill(getBx(), (uint16_t)((getSymPosY() < getBy() + fontHeight) ? getBy() : getSymPosY() - (fontHeight - 1)),
                       (uint16_t)(getSymPosX() - 1), (uint16_t)getSymPosY(),
                       (uint16_t)getGroundColor());

          for(;;)
          {
              uint16_t val;
              val = GETCHAR(stringMarks->begin);

              if (isAlignH() && ((val == '\r') || (val == '\t')))
                  val = ' '; /* Process \t and \r as space in horizontal alignment modes */

              putChar( val );
              if (stringMarks->begin == stringMarks->end)
                  break;
              GINCPTR(stringMarks->begin);
          }

          /* Clear viewport to the left of the string segment ? */
          if (isVpClrRight() && (getSymPosX() <= getEx()) && !isTransmode)
                  fill((uint16_t) getSymPosX(),(uint16_t)((getSymPosY() < getBy() + fontHeight) ? getBy() : getSymPosY() - (fontHeight - 1)),
                       getEx(), getSymPosY(), (uint16_t)getGroundColor());

          if (ln == 3)
          {
              if (processNewline(fontHeight)) /* Make new line processing */
                  break;  /* a no-scroll condition reached, no more characters needed */
          }
          ln = sizeStringSeg(stringMarks);
          }
          while (ln != 0);
   }
   else
   {
       /* The string only contained whites, or was empty */
       if ((isVpClrLeft() || isVpClrRight()) && !isTransmode)
       {
           /* clear of empty line */
           fill((uint16_t)(isVpClrLeft() ? getBx() : getSymPosX()),
                    (uint16_t)((getSymPosY() < getBy() + fontHeight) ? getBy() : getSymPosY() - (fontHeight - 1)),
                    (uint16_t)(isVpClrRight() ? getEx() : getSymPosX() - 1),
                    (uint16_t)getSymPosY(),(uint16_t)getGroundColor());
       }
   }
   /* Clear viewport from line to bottom ? */
   if (isVpClrDown() && (getSymPosY() < getEy()) && !isTransmode)
      fill(getBx(), getSymPosY() + 1, getEx(), getEy(), (uint16_t)getGroundColor());

   putComplete();
}

/*
    Выводит символ на экран,
    предполагая, что он доступен для печати
    ('\n' и '\r' обрабатываются на более высоком уровне)

    Возвращает 0, если печатается
    Возвращает ширину символа, если не печатается (не места в vp, и ! GLINECUT)
*/

char GPainter::putChar( uint8_t val )
{
    uint16_t w;
    if ((val == (uint8_t)'\n') || (val == (uint8_t)'\r'))
        return 0; /* No output, positions are handled at the level above */
    if (val == (uint8_t)'\t')
    { /* Tabulator is handled like a single variable width symbol */
        if ((w = tabStep(getSymPosX())) != getBx())
        {
            uint16_t fh = gfGetFontH(  curViewPort->font );
            fill(getSymPosX(), (uint16_t)((getSymPosY() < getBy() + fh) ? getBy() : (getSymPosY() - (fh - 1))),
                     w, getSymPosY(),(uint16_t)getGroundColor());
            setSymPosX(w); /* Move position to tab setting or end of viewport */
        }
    }
   else
   {
       GSymbol* psymbol; /* pointer to a symbol */
       uint16_t fh;
       fh = gfGetFontH( curViewPort->font );

       if (getSymPosX() > getEx())
       {
           setSymPosX(getEx() + 1);
           return 0; /* Line overflow by previous char, skip (waiting for \n or \r) */
       }

       psymbol = getSymbol( val , curViewPort->font, curViewPort->codePage);

       if( psymbol == NULL )
       {
           // G_WARNING( "gputch: Character have undefined symbol" );
           return 0;
       }
       w = gsymW(psymbol);
       if (!isPartialLine() && ((uint16_t) getSymPosX() + w - 1 > (uint16_t) getEx()))
           return 1; /* not room for symbol, skip to avoid cut */

       putSymbol( (uint16_t)getSymPosX(),
                  (uint16_t)((getSymPosY() + 1) - fh),
				  (uint16_t)getEx(),
				  (uint16_t)getEy(),
                  (GSymbol*)psymbol,
                  (uint16_t)(( fh <= getSymPosY() - getBx() ) ? 0 : fh - ((getSymPosY() - getBx()) + 1)), /* yoffset*/
			      (uint16_t)getSymSize(curViewPort->font) );

       if ((uint16_t) getSymPosX() + w > (uint16_t) getEx())
           setSymPosX(getEx());
       else
       {
           setSymPosX(getSymPosX() + w);
       }
   }
   return 0;
}

uint8_t GPainter::processNewline(uint16_t lnsp)
{
    if (lnsp == 0)
        lnsp = gfGetFontH( curViewPort->font );

    if (((uint16_t) getSymPosY() + lnsp) <= ((uint16_t)getEy()))
    {
        updatehw();   /* Update hardware here to speed buffered mode */
        setSymPosY(getSymPosY() + lnsp);    /* Inside vp area, just advance position */
    }
    else
    {
        if (isNoScroll())
        {
            /* No scroll mode enabled for viewport (or hardware does not support read) */
            if ( isFullHeightLine || (getSymPosY() >= getEy()))
                return 1;
            /* else Room for a partial line below current line */
            setSymPosY(getSymPosY() + lnsp); /* Ok that cposy exceeds viewport here to compensate for ancher position */
        }
        else
        {
            uint16_t numlines;
            if ( isFullHeightLine )
            { /* Scroll so relative line positions are the same */
                numlines = lnsp;
            }
            else
            { /* Scroll so line is aligned with viewport bottom */
                numlines = (uint16_t)(((uint16_t)getSymPosY() + lnsp) - getEy());
                setSymPosY(getEy());
            }

            /* Activate viewport scroll */
            scroll(
                        getBx(),
                        getBy(),
                        getEx(),
                        getEy(),
                        numlines,
                        (uint16_t)getGroundColor());
        }
    }
    setSymPosX(getBx());
    return 0;
}

void GPainter::scroll(uint16_t ltx, uint16_t lty, uint16_t rbx, uint16_t rby, uint16_t lines, uint16_t pattern)
{
//    uint16_t ys;
//    GColor *cp;
//    uint16_t ylim;
//    uint16_t x;
//
//    //glcd_err = 0;
//
//    /* Force resoanable values */
//    GLIMITU(ltx, display->getWidth() - 1);
//    GLIMITU(lty, display->getHeight()-1);
//    GLIMITD(rby, lty);
//    GLIMITU(rby, display->getHeight()-1);
//    GLIMITD(rbx, ltx);
//    GLIMITU(rbx, display->getWidth()-1);
//
//    /* Non-buffered mode */
//    if (lines > rby - lty)
//    {
//        fill(ltx, lty, rbx, rby, pattern);   /* just clear whole area */
//        return;
//    }
//
//    ylim = rby - lines;
//    ys = (lty + lines);        /* First source row for scroll */
//
//    for ( ; lty <= rby; lty++, ys++)
//    {
//        /* Loop rows */
//        if (lty > ylim)
//        {
//            fill(ltx, lty, rbx, rby, pattern);   /* clear remaining area */
//            return;
//        }
//        //setxypos(ltx,ys);
//        ghw_auto_rd_start();
//        /* Loop pixel row read */
//        for (x = ltx, cp = &tmpbuf[0]; x <= rbx; x++,cp++)
//            *cp = ghw_auto_rd();
//        //ghw_setxypos(ltx,lty);
//
//        /* Loop pixel row write */
//        for (x = ltx, cp = &tmpbuf[0]; x <= rbx; x++,cp++)
//            display->autoWrite(*cp);
//    }
}

void GPainter::calcDataCheck()
{
    uint8_t i,s;
    s = (uint8_t)((GViewPort*)&(curViewPort->check) - curViewPort);
    curViewPort->check = 0;
    for ( i = 0; i < s; i++)
    {
        curViewPort->check += ((uint8_t*)curViewPort)[i];
    }
}

void GPainter::putComplete()
{
    /* update viewport cursor data */
    if (getSymPosX() > getEx()) setSymPosX(getEx());
    if (getSymPosY() > getEy()) setSymPosY(getEy());

    setPixPosX(getSymPosX()); /* update graphics pos also */
    setPixPosY(getSymPosY());

    updatehw();
    calcDataCheck(); /* correct VP to new settings */
}

/*
   gputchw  use these mode setting attributes

      GNORMAL,        Нормальный режим (не инвертированный, без выравнивания)
      GINVERSE        Инвертированный цвет (типичный белый на черном)
      GNOSCROLL       Прокрутка на конце видового экрана подавлена
      GNO_WRAP        Символы не переносятся (только \n обрабатывается)
      GPARTIAL_LINE   Показывать только строки, где видна полная высота символа
      GPARTIAL_CHAR   Показывать только символы, где видна полная ширина символа
      GTRANSPERANT    Задний фон символа прозрачный

      gputch  maps to gputchw using an appropriate cast
*/

void GPainter::putCharW( uint8_t val )
   {
   putPrepare();

   switch( val )
      {
      case ((uint8_t)'\n'):
         /* Make new line processing */
         processNewline(0);
         break;
      case ((uint8_t)'\r'):
         setSymPosX(getBx());
         break;
      default:
         {
         if (putChar( val )) /* process character, incl '\t' */
            {
            /* end of viewport line reached, character put skipped */
            if (!isNoWrap())
               {
               if (processNewline(0)) /* Make new line processing */
                  break;  /* a no-scroll condition reached, no more characters needed */
               putChar( val ); /* Retry on new position */
               }
            }
         }
      }

   putComplete();
   }

void GPainter::putPrepare(void)
   {
  // gi_datacheck(); /* check internal data for errors */
 //  glcd_err = 0;   /* Reset HW error flag */

//   gi_cursor( 0 ); /* kill cursor */

      isFullHeightLine = !isPartialLine();
   }


uint8_t GPainter::sizeStringSeg( StringMarks* stringMarks )
   {
   uint16_t sw; /* symbol width */
   const char *cp, *cpsp, *cpcstart;
   uint16_t slensp = 0;
   uint8_t val;
   uint8_t first;
   cp = stringMarks->next;
   if (GETCHAR(cp) == 0)
      return 0;       /* fast skip of last string segment */

   if (isAlignHR())
   {
       /* Skip leading whites */
       for(;;)
       {
           val = GETCHAR(cp);
           if (!(/*(val == ' ') || */(val == '\t') || (val == '\r'))) /* leading white ? */
           {
               break;
           }
           GINCPTR(cp);
       }
       setSymPosX(getBx());  /* cposx should not participate in width evaluation */
   }
   else
      {
      if (isAlignLeft() || isAlignBottom())
         setSymPosX(getBx());   /* Start at left edge */
      }

   stringMarks->begin = cp;  /* Begin of string */
   stringMarks->end = cp;
   cpsp  = (const char*)((void*)NULL);    /* No word separator detected */
   stringMarks->len = 0;        /* Length = 0 */

   for (first = 1 ; ; first = 0)
   {
       cpcstart = cp; /* Mark start of (multibyte) character */
       val = GETCHAR(cp);
       if ((val == (uint8_t) 0) || (val == (uint8_t) '\n'))
           break;  /* End of line segment reached */

       if (val == (uint8_t)'\r')
       {
           if (isAlignH())
           {
               val = (uint8_t)' ';  /* Process \r as space in horizontal alignment modes */
               goto process_space;
           }
           else
           {
               if (first)  /* Process \r only as first character */
               {
                   setSymPosX(getBx());
                   goto skipchar;
               }
               break;  /* \r end of line segment */
           }
       }

       if (val == '\t')
       { /* Tabulator is handled as a single variable width symbol */
           if (isAlignH())
           {
               val = (uint8_t)' ';  /* process \t as space in horizontal alignment modes */
               goto process_space;
           }
           cpsp = stringMarks->end;      /* Mark word boundary at previous (or first character) */
           slensp = stringMarks->len;
           sw = tabStep((uint16_t)(getSymPosX() + stringMarks->len)) -  getSymPosX(); /* Viewport tab position in viewport */
           sw -= stringMarks->len;            /* Virtual tab character width */
       }
       else
       {
           if (val == ' ')
           {
process_space:
               cpsp = stringMarks->end;      /* Mark word boundary at previous (or first) character */
               slensp = stringMarks->len;
           }

           /* Get width of (multibyte) character */
           sw = getSymWidth(val);       /* = GDISPCW ifndef GGSOFT_FONT */
       }

       if (sw >= (getEx() - getBx() + 1))
       {                         /* (first) symbol larger than viewport */
           stringMarks->len = ((uint16_t)(getEx() - getBx()) + 1);   /* Limit size (for calculations) to within viewport */
           GINCPTR(cp);              /* Point to next character */
           cpcstart = cp;            /* Mark start of (multibyte) character */
           break;
       }
       if (isPartialChar())
       {
           if ((stringMarks->len + getSymPosX() + (isAlignRight() ? sw - 1 : 0)) > getEx())
               break;  /* reached last (partly) visible character on line */
       }
       else
       {
           if (((stringMarks->len + sw) - 1) + getSymPosX() > getEx())
           {
               /* Word exceeds viewport */
               if ((cpsp != NULL) && isWordWrap())
               {
                   /* Break word at previous word boundary */
                   stringMarks->end = cpsp;
                   stringMarks->len = slensp;
                   cp = cpsp;
                   GINCPTR(cp); /* Move point to word separator */
                   GINCPTR(cp); /* Move point to character after word separator */
                   cpcstart = cp; /* Mark start of (multibyte) character */
               }

               /* else Break word at character boundary */
               break;
           }
       }
       stringMarks->len += sw;

skipchar:

       stringMarks->end = cp;
       GINCPTR(cp); /* Move point to character after word separator */
       cpcstart = cp; /* Mark start of (multibyte) character */
   }

   /* Set next character */
   stringMarks->next = cpcstart;  /* Set start of (multibyte) character
   (for rewind if the current character is not used due to wrapping ) */
   if (GETCTRLCHAR(stringMarks->begin) == 0)
      return 0;  /* Empty line segment */

   if ((GETCTRLCHAR(stringMarks->next) != 0)|| isAlignCenterH())
      {
      /* skip trailing whites from line segment */
      val = GETCTRLCHAR(stringMarks->end);
      if (((val == ' ') || (val == '\t')) &&
           (stringMarks->end != stringMarks->begin) && (cpsp != NULL))
         {
         stringMarks->end = cpsp;
         stringMarks->len = slensp;
         }
      }

   for (;;)
      {
      val = GETCTRLCHAR(stringMarks->next);
      if (!isWordWrap())
         break;
      /* Linecut is active, skip to end of line or end of string */
      if ((val == 0) || (val == '\n'))
         break;
      GINCPTR(stringMarks->next);
      }

   if (val == 0)
      return 1;
   if (val == '\r')
      return 2;
   if ((val == '\n') || (val == '\t'))
      GINCPTR(stringMarks->next);
   return 3;
}

void GPainter::fill(uint16_t ltx, uint16_t lty, uint16_t rbx, uint16_t rby, uint16_t pattern)
{
    uint16_t y;
    uint16_t x;

    //isError = 0;

    /* Force reasonable values */
    GLIMITU(ltx, display->getWidth()  - 1);
    GLIMITU(lty, display->getHeight() - 1);
    GLIMITD(rby, lty);
    GLIMITU(rby, display->getHeight() - 1);
    GLIMITD(rbx, ltx);
    GLIMITU(rbx, display->getWidth() - 1);

    display->setViewPort({ltx, lty, rbx, rby});

    if ((pattern == 0) || (pattern == 0xffff) || (pattern == 0xff))
    {
        /* Accelerated loop fill for uniform color */
        GColor c = (pattern != 0) ? getFrColor() : getBgColor();
        for (y = lty; y <= rby; y++)
        {
            x = rbx - ltx;
            do
            {
                display->autoWrite(c);
            }
            while (x-- != 0);
        }
    }
    else
    {
        const char sympixmsk[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01}; // seps525
        /* loop fill for pattern background (mixed foreground / background) */
        for (y = lty; y <= rby; y++)
        {
            uint8_t msk, pat;
            pat = ((y & 1) != 0) ? (uint8_t)(pattern / 256) : (uint8_t)(pattern & 0xff);
            msk = sympixmsk[GPIXEL(ltx)];
            for (x = ltx; x <= rbx; x++ )
            {
                display->autoWrite(((pat & msk) ? getFrColor() : getBgColor()));
                if ((msk >>= 1) == 0)
                    msk = sympixmsk[GPIXEL(x+1)];
            }
        }
    }
}

uint16_t GPainter::tabStep(uint16_t x)
{
    x -= getBx();
    /* find next tab */
    x =((x / tabSize) + 1) * tabSize + getBx();
    return x;
}

uint16_t GPainter::getSymWidth( uint8_t c )
{
    GSymbol *ps;

    if (curViewPort == NULL)
        return GDISPCW;
    if (curViewPort->font == NULL)
        return GDISPCW;

    ps = getSymbol(c, curViewPort->font, curViewPort->codePage);
    if( ps == NULL )
        return 0;
    return gsymW(ps); /* Width of SW font */
}

GSymbol* GPainter::getSymbol( uint8_t c, GFont* pfont, GCodePage* codepagep)
{
    uint8_t* ps;
    uint16_t s;
    const GCodePageRange* crpb;
    uint8_t numelem, min, half;
    const GCodePageRange* crp;

    if( codepagep != NULL )
    {
        /* Using codepage, convert character to an index via lookup */
        crpb = &codepagep->range[0]; /* Codepage element array base */

        if (codepagep->header.codePageRangeNumber == 0)
        {
           // G_WARNING("Illegal codepage header detected");
            return NULL;
        }

        /* Use fast lookup
    Much faster with large codepages but require that codepage elements are
    arranged in increasing order (= default ordering for all standard font
    code pages, and for the fonts codepages created with IconEdit) */
        for(;;)
        {
            /* Sub range = full range */
            numelem = codepagep->header.codePageRangeNumber;
            min = 0;
            for(;;)
            {
                half = numelem >> 1;   /* half = mid point in sub range */
                crp = &crpb[(uint16_t)min + half]; /* Next element to check */
                if ((c >= crp->min) && (c <= crp->max))
                {
                    /* Found */
                    c = crp->index + (c - crp->min); /* Convert to symbol index */
                    goto c_converted; /* fast skip of all loops */
                }
                else
                {
                    if (half != 0)
                    {
                        /* define next sub range */
                        if (c < crp->min)
                            numelem = half;        /* too high, continue in lower half */
                        else
                        {
                            min = min + half;        /* too low, continue in upper half */
                            numelem = numelem - half;
                        }

                        if (numelem != 0)        /* subrange exist ? */
                            continue;
                    }

                    /* Not found */
                    if (c == codepagep->header.defaultChar)
                    {
                        /* Default character does not exist (either) */
                        //G_WARNING("Illegal codepage default character detected");
                        return NULL;
                    }
                    else
                    {
                        /* Use default character instead */
                        c = codepagep->header.defaultChar;
                        break; /* Search once more in full range, skip inner loop */
                    }
                }
            }
        }
    }
    else
        if ( c > getNumSym(pfont))
        {
            //G_WARNING( "gfsymv.c: symbol index larger than font table" );
            return NULL;
        }

c_converted:

    /* Is a standard font (linear symbol array in linear memory) */
    s = ((uint16_t)(giscolor(pfont->symbols) ? sizeof(GColorSymHead) : sizeof(GSymHead))) /* Sizeof symbol header */
            + ((uint16_t) pfont->symSize); /* size of symbol data */
    ps = (uint8_t*) (pfont->symbols);
    return (GSymbol*)(&ps[s * c]);
}


/************************* ggrect.c ********************************

   Рисует округленный прямоугольник.
   Любое морфинг между прямоугольником и кругом может быть нарисовано.
   Если радиус = 0, то рисуется прямоугольник.
   Для квадратичной фигуры с радиусом r >= width/2 фигура будет кругом.

   С помощью флагов дуги (skipflags) можно выбрать нормальный угол 90 градусов
   вместо указанных скругленных углов:

      GCARC_LT   left-top corner
      GCARC_LB   left-bottom corner
      GCARC_RT   right top corner
      GCARC_RB   right bottom corner.

   Тип рисования выбирается (or'ed with skipflags) :

     GLINE  Perimeter line with foreground color (default)
     GFILL  Inner area filled with background info
     GFRAME Combination of above

    Округленный прямоугольник состоит из:
	четырех (0-4) дуговых областей LT, LB, RT, RB
    и трех (0-3) прямоугольных областей: MID, LC, RC.
    Если одна или несколько областей дуги не используются,
    тогда прямоугольные области оптимизированы соответственно.
    Прямоугольные области с нулевым размером пропускаются
    (например, когда комбинированные дуги образуют правильный круг)

      **********
     * *      * *
    *LT*      *RT*
    ****      ****
    *LC*  MID *RC*
    ****      ****
    *LB*      *RB*
     * *      * *
      **********

*********************************************************************/

void GPainter::drawRoundRect( uint16_t ltx, uint16_t lty, uint16_t rbx, uint16_t rby, uint16_t r, uint8_t skipflags)
   {
   uint16_t ycb, yce;
   uint16_t xb, xe;
   uint16_t yb, ye;
   GColor coll;
   uint16_t pattern;
   bool isFill, isLine;

  // gi_datacheck();

   /* normalize to view-port */
   ltx += getBx();
   lty += getBy();
   rbx += getBx();
   rby += getBy();

   /* limit values to view-port */
   limitToVP(&ltx, &lty, &rbx, &rby );

  // glcd_err = 0; /* Reset HW error flag */


   /* Check special cases */
   isLine = (skipflags & 0x1) ? 0 : 1; /* GLINE or GFRAME */
   isFill = (skipflags & 0x2) ? 1 : 0; /* GFILL or GFRAME */
   if (!isLine && !isFill)
      return;  /* Neither frame nor filled area -> no output, skip drawing */

   xb = (rbx - ltx + 1);
   yb = (rby - lty + 1);
   if ((xb <= 2) || (yb <= 2) || ((skipflags & GCARC_ALL) == GCARC_ALL))
      r = 0; /* All corner arcs are to be skipped = rectangle */
   else
   {
       /* Check / limit radius (maximum is two half circles or one full circle */
       if ((uint16_t)(2 * r) > (uint16_t)xb)
           r = (uint16_t)(xb / 2) + (r & 1);
       if ((uint16_t)(2 * r) > (uint16_t)yb)
           r = (uint16_t)(yb / 2) + (r & 1);
   }

   /* Set colors */
   pattern = !getGroundColor(); //isInverse() ? 0xffff : 0x0000;
   coll = getGroundColor();

   if (r <= 0)
   {
       /* Speed optimized for single rectangular area */
       if (isLine)
       {
           drawRectangle( ltx, lty, rbx, rby, coll );
           if ((rbx - ltx <= 1) || (rby - lty <= 1))  /* No inner area */
               return;
           ltx++;
           lty++;
           rbx--;
           rby--; /* Limit fill to inner area */
       }
       if (isFill)
           fill(ltx, lty, rbx, rby, pattern);
       updatehw();
       return;
   }

   /* Set partial left areas */
   ycb = lty;
   yce = rby;
   xb = ltx;
   if ((skipflags & (GCARC_LT | GCARC_LB)) != (GCARC_LT | GCARC_LB))
   { /* Some arcs on left side */
       xe = ltx + r;
       if ((skipflags & GCARC_LT) == 0)
       {  /* Upper left arc */
           yb = lty;
           ye = lty + r;
           drawCornerArc(xe, ye, r, (uint8_t)(GCARC_LT | (skipflags & GFILL)));
           ycb = ye + 1;
       }
       if ((skipflags & GCARC_LB) == 0)
       {  /* Lower left arc */
           ye = rby;
           yb = rby - r;
           drawCornerArc(xe, yb, r, (uint8_t)(GCARC_LB | (skipflags & GFILL)));
           yce = yb - 1;
       }
       if (yce >= ycb)
       {
           /* Left rectangle */
           if (isLine)
           {
               if (skipflags & GCARC_LT)
               { /* Upper left line */
                   drawRectangle(xb, ycb, xe, ycb, coll );
                   ycb++;
               }
               if (skipflags & GCARC_LB)
               {  /* Lower left line */
                   drawRectangle(xb, yce, xe, yce, coll );
                   yce--;
               }
               if (yce >= ycb)
               {  /* left vertical line*/
                   drawRectangle(xb, ycb, xb, yce, coll );
                   xb++;
               }
           }
           if (isFill)
           {  /* Fill left rectangle */
               if ((yce >= ycb) && (xe >= xb))
                   fill(xb, ycb, xe, yce, pattern );
           }
       }
       xb = xe + 1;
   }
   else
   {
       xb = ltx;
       if (isLine)
       {
           drawRectangle(xb, lty, xb, rby, coll );
           xb++;
       }
   }

   /* Mid rectangle */
   xe = ((skipflags & (GCARC_RT | GCARC_RB)) != (GCARC_RT | GCARC_RB)) ? (rbx - r) - 1 : rbx;
   if (xe >= xb)
   {
       ycb = lty;
       yce = rby;
       if (isLine)
       {
           drawRectangle(xb, ycb, xe, ycb, coll );
           ycb++;
           drawRectangle(xb, yce, xe, yce, coll );
           yce--;
       }
       if (isFill)
       {  /* Fill rectangle */
           if (yce >= ycb)
               fill(xb, ycb, xe, yce, pattern );
       }
   }

   /* Set partial right areas */
   ycb = lty;
   yce = rby;
   xe = rbx;
   if ((skipflags & (GCARC_RT | GCARC_RB)) != (GCARC_RT | GCARC_RB))
   { /* Some arcs on right side */
       xb = rbx - r;
       if ((skipflags & GCARC_RT) == 0)
       {  /* Upper right arc */
           yb = lty;
           ye = lty + r;
           drawCornerArc(xb, ye, r, (uint8_t)(GCARC_RT | (skipflags & GFILL)));
           ycb = ye + 1;
       }
       if ((skipflags & GCARC_RB) == 0)
       {  /* Lower right arc */
           yb = rby - r;
           drawCornerArc(xb, yb, r, (uint8_t)(GCARC_RB | (skipflags & GFILL)));
           yce = yb - 1;
       }
       if (yce >= ycb)
       {
           /* Left rectangle */
           if (isLine)
           {
               if (skipflags & GCARC_RT)
               { /* Upper left line */
                   drawRectangle(xb, ycb, xe, ycb, coll );
                   ycb++;
               }
               if (skipflags & GCARC_RB)
               {  /* Lower left line */
                   drawRectangle(xb, yce, xe, yce, coll );
                   yce--;
               }
               if (yce >= ycb)
               {  /* left vertical line*/
                   drawRectangle(xe, ycb, xe, yce, coll );
                   xe--;
               }
           }
           if (isFill)
           {  /* Fill left rectangle */
               if ((yce >= ycb) && (xe >= xb))
                   fill(xb, ycb, xe, yce, pattern );
           }
       }
   }
   else
   {
       if (isLine)
       {
           drawRectangle(xe, ycb, xe, yce, coll );
       }
   }
   updatehw();
}

void GPainter::drawRectangle(uint16_t ltx, uint16_t lty, uint16_t rbx, uint16_t rby, GColor color)
{
    //glcd_err = 0;

    GLIMITU(ltx, display->getWidth() - 1);
    GLIMITU(lty, display->getHeight()-1);
    GLIMITD(rby, lty);
    GLIMITU(rby, display->getHeight()-1);
    GLIMITD(rbx, ltx);
    GLIMITU(rbx, display->getWidth()-1);

    if (ltx != rbx)
        drawLineH(ltx, lty, rbx, color);      /* Draw horisontal line */

    if (lty != rby)
    {
        drawLineV(ltx, lty, rby, color);      /* Draw vertical line */
        if (ltx != rbx)
        {                                  /* It is box coordinates */
            drawLineH(ltx, rby, rbx, color);   /* Draw bottom horizontal line */
            drawLineV(rbx, lty, rby, color);   /* Draw right vertical line */
        }
    }
    else
        if (ltx == rbx)
            drawLineH(ltx, rby, rbx, color);   /* Draw dot */
}

void GPainter::drawLineH(uint16_t xb, uint16_t yb, uint16_t xe, GColor color)
{
    display->setViewPort({xb, yb, xe, yb});
    for ( ; xb <= xe; xb++ )
        display->autoWrite(color);  /* Write destination */
    display->setHautoInc();
    display->update();
}

void GPainter::drawLineV(uint16_t xb, uint16_t yb, uint16_t ye, GColor color)
{
    display->setViewPort({xb, yb, xb, ye});
    while (yb <= ye )
    {
        display->autoWrite(color);
        yb++;
    }
    display->setVautoInc();
    display->update();
}

/************************* gcarc.c ********************************

   Рисует угловую дугу (90 градусная дуга, выровненная по x,y осям)

   Возможности рисования:

     GLINE  	Линия по периметру c foreground color (default)
     GFILL  	Внутренняя область заливается согласно background color
     GFRAME 	Комбинация обоих

   Ориентация дуги выбирается с этими параметрами

     GCARC_LT   left-top corner
     GCARC_LB   left-bottom corner
     GCARC_RT   right top corner
     GCARC_RB   right bottom corner.

  Одно или несколько направлений дуги рисуются одним вызовом.

*********************************************************************/

void GPainter::drawCornerArc(uint16_t xc, uint16_t yc, uint16_t r, uint8_t arctype)
   {
//   GXYT x,x1;
//   GXYT y,y1;
//   SGUCHAR mx,my,fill,line;
//   GCOLOR colf,coll;
//   SGLONG p;
//
//   line = ((arctype & 0x1)!=0) ? 0 : 1; /* GLINE or GFRAME */
//   fill = ((arctype & 0x2)!=0) ? 1 : 0; /* GFILL or GFRAME */
//   coll = G_IS_INVERSE() ? ghw_def_background : ghw_def_foreground;
//   colf = G_IS_INVERSE() ? ghw_def_foreground : ghw_def_background;
//
//   /* Process one or more arc types */
//   for(;;)
//      {
//      if ((arctype & GCARC_LT)!=0)
//         {
//         arctype &= ~GCARC_LT;
//         my=1;
//         mx=1;
//         }
//      else
//      if ((arctype & GCARC_LB)!=0)
//         {
//         arctype &= ~GCARC_LB;
//         my=0;
//         mx=1;
//         }
//      else
//      if ((arctype & GCARC_RT)!=0)
//         {
//         arctype &= ~GCARC_RT;
//         my=1;
//         mx=0;
//         }
//      else
//      if ((arctype & GCARC_RB)!=0)
//         {
//         arctype &= ~GCARC_RB;
//         my=0;
//         mx=0;
//         }
//      else
//         return;
//
//      /* Start on new arc */
//      x = 0;
//      y = r;
//      p = (SGLONG)((SGINT)1 - (SGINT)r);
//      x1 = x;
//      y1 = y;
//
//      /* Calculate a 45 degree angle, and mirror the rest */
//      for(;;)
//         {
//         if (line)
//            {
//            if (y != x)
//               {
//               ghw_setpixel((GXT)MIRX(xc,x),(GYT)MIRY(yc,y),coll);
//               if (fill && (y != y1))
//                  {
//                  /* Fill using horizontal lines, so check on vertical move */
//                  drawRectangle((GXT)SWPX(SUB(xc,x-1),xc),
//                                (GYT)MIRY(yc,y),
//                                (GXT)SWPX(xc,ADD(xc,x-1)),
//                                (GYT)MIRY(yc,y),colf);
//                  y1=y;
//                  }
//               }
//            ghw_setpixel((GXT)MIRX(xc,y),(GYT)MIRY(yc,x), coll);
//            if (fill)
//               drawRectangle((GXT)SWPX(SUB(xc,y-1),xc),
//                             (GYT)MIRY(yc,x),
//                             (GXT)SWPX(xc,ADD(xc,y-1)),
//                             (GYT)MIRY(yc,x),colf);
//            if (x+1 >= y)
//               break;
//            }
//         else
//            {
//            drawRectangle((GXT)SWPX(SUB(xc,y),xc),
//                          (GYT)MIRY(yc,x),
//                          (GXT)SWPX(xc,ADD(xc,y)),
//                          (GYT)MIRY(yc,x),colf);
//            if (y != y1)
//               {
//               finalize:
//               drawRectangle((GXT)SWPX(SUB(xc,x1),xc),
//                          (GYT)MIRY(yc,y1),
//                          (GXT)SWPX(xc,ADD(xc,x1)),
//                          (GYT)MIRY(yc,y1),colf);
//               y1=y;
//               }
//            x1=x;
//            if (x+1 >= y)
//               {
//               if (x!=y)
//                  {
//                  x=y;
//                  goto finalize;
//                  }
//               break;
//               }
//            }
//
//         /* Calculate next arc point */
//         if( p < 0 )
//            {
//            x = x + 1;
//            p = p + ((SGLONG)2) * ((SGLONG)x) + 1;
//            }
//         else
//            {
//            x = x + 1;
//            y = y - 1;
//            p = p + ((SGLONG)2)*((SGLONG)((SGINT)x - (SGINT)y)) + 1;
//            }
//         }
//      }
   }

void putSymbol(uint16_t xs, uint16_t ys, uint16_t xemax, uint16_t yemax, GSymbol *psymbol, uint16_t yoffset, uint32_t symsize)
   {
//   SGUINT xe;
//   SGUINT ye;
//   SGUINT bw;
//   SGUCHAR mode; /* hw mode flags */
//   PGSYMBYTE f;
//
//   xe = (SGUINT) gsymw(psymbol);
//   ye = (SGUINT) gsymh(psymbol);
//   if ((ye == 0) || (xe == 0))
//      {
//      /* Error: illegal symbol header data. Corrupted data or a ROM/RAM pointer problem detected */
//      G_WARNING( "Illegal symbol header data detected. Zero sized symbol" );
//      #ifdef GVIRTUAL_FONTS
//      if (gissymbolv(psymbol) && (xe == 0))
//         {
//         G_WARNING("Check if virtual font has been opened correctly");
//         }
//      #endif
//      return;
//      }
//   if (giscolor(psymbol))
//      {
//      SGBOOL greymode;
//      #ifdef GVIRTUAL_FONTS
//      if (gissymbolv(psymbol))
//         {
//         #ifdef GDATACHECK
//         /* Check for mixed font design error */
//         if (((((PGSYMHEADV)psymbol)->type_id) & GVTYPEMASK) != 1)
//            { /* Error: format is unknown to this library */
//            G_WARNING( "Unsupported extended font format. Illegal virtual font" );
//            return;
//            }
//         #endif
//         f = (PGSYMBYTE) NULL; /* Signal use of virtual font to low-level driver*/
//         }
//      else
//      #endif
//         f = (PGSYMBYTE)&(((PGCSYMBOL)psymbol)->b[0]);  /* first byte in color symbol */
//      mode = (SGUCHAR) gcolorbits(psymbol);
//      greymode = ((mode & 0xc0) == 0x40) ? 1 : 0;
//      mode &= 0x3f;
//      if (symsize == 0)
//         {
//         if (mode <= 8)
//            bw = (xe*((SGUINT)  mode)+7)/8;    /* width of symbol in bytes */
//         else
//            bw =  xe*((((SGUINT)mode)+7)/8);   /* width of symbol in bytes */
//         }
//      else
//         bw = (SGUINT)(symsize/ye);
//      if (mode >= 32) mode = 0x1f;
//      if ((greymode != 0) && (mode <= 8))
//         {
//         mode |= GHW_GREYMODE;
//         if (G_IS_INVERSE())
//            mode |= GHW_INVERSE;
//         }
//      }
//   else
//      {
//      #ifdef GVIRTUAL_FONTS
//      if (gissymbolv(psymbol))
//         {
//         #ifdef GDATACHECK
//         /* Check for mixed font design error */
//         if (((((PGSYMHEADV)psymbol)->type_id) & GVTYPEMASK) != 1)
//            { /* Error: format is unknown to this library */
//            G_WARNING( "Unsupported extended font format. Illegal virtual font" );
//            return;
//            }
//         #endif
//         f = (PGSYMBYTE) NULL; /* Signal use of virtual font to low-level driver*/
//         }
//      else
//      #endif
//         f = (PGSYMBYTE)&(((PGBWSYMBOL)psymbol)->b[0]); /* first byte in B&W symbol */
//      mode = G_IS_INVERSE() ? GHW_INVERSE : 0;
//      if (symsize == 0)
//         bw = (SGUINT) (xe+7)/8;  /* width of symbol in bytes */
//      else
//         bw = (SGUINT)(symsize/ye);
//      }
//
//    if (G_IS_TRANSPERANT())
//       mode |= GHW_TRANSPERANT;
//
//   if (yoffset != 0)
//      {                          /* Only showing lower part of symbol */
//      ys += yoffset;
//      ye -= yoffset;
//      #ifdef GVIRTUAL_FONTS
//      if (!gissymbolv(psymbol))
//      #endif
//         f=&f[bw*yoffset];
//      }
//
//   xe = xe+((SGUINT) xs)-1;
//   ye = ye+((SGUINT) ys)-1;
//
//   /* truncate at max rect */
//   if( xe > ((SGUINT) xemax) )
//      xe = ((SGUINT) xemax);
//   if( ye > ((SGUINT) yemax) )
//      ye = ((SGUINT) yemax);
//
//   glcd_err = 0; /* Reset HW error flag */
//
//   #ifdef GVIRTUAL_FONTS
//   if (gissymbolv(psymbol))
//      gi_symv_open( psymbol, bw, yoffset ); /* Preset virtual symbol interface */
//   #endif
//   ghw_wrsym(xs, ys, (GXT) xe, (GYT) ye, f, bw, mode );
   }

