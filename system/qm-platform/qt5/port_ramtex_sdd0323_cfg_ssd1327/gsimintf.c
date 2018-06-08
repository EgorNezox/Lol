/****************************** GSIMINTF.C *****************************

   Socket interface to the external LCD simulator program.
   Commands and data to/from the simulator is send via sockets messages.

   If the LCD application is developed as a console application (recommended)
   the following library file must be included in the link
      For versions >= MSVC 6.x and Borland C++ 5.0 :  WSOCK32.LIB

   Revision date:     11-10-04
   Revision Purpose:  GSimFlush updated for segmented transmission to
                      handle large graphic LCD screens.
   Revision date:     171204
   Revision Purpose:  RGB mode optimized, buffer allocations optimized
   Revision date:     190407
   Revision Purpose:  Touch screen simulation interface added.
   Revision date:     080808
   Revision Purpose:  Fix connection error when MSVC unicode string modes were used
   Revision date:     030109
   Revision Purpose:  Fix flush error with large video buffers.
   Revision date:     20-07-09
   Revision Purpose:  Assure process release for Simulator in tight touch poll loops
   Revision date:     22-01-15
   Revision Purpose:  General commands update to include LCDserv.exe version 3.0 extensions.
                      All used commands now waits for a server response (this command level
                      handshake always keeps LCDserv.exe screen and LCD application in sync)
                      Possible RX timeout handling can be added via the switch RX_TIMEOUT
                      in this module. If defined, then server communication use receive timeout
                      with possible connection retry.
                      Multi-Touch screen simulation interface added.
                      New application interface functions:
                         GSimStatus()        Report keyboard and touch states in one fast operation.
                         GSimTouchInfo(..)   Fetch extended touch info. Single touch or multi-touch data.
                                             New switch GTOUCH_MAX_PRESSED (can be globally set) defines max
                                             number of emulated touch positions (1-5).
                         GSimTouchSetup(..)  Modify touch screen behaviour and multi-touch emulation keys.
                         GSimSync()          Assure Client/LCDSERV.EXE PC screen timing syncronization, fast.
                         GSimVersion()       Report LCDSERV.EXE version (for future release use).
   Revision Purpose:  gdispcfg.h now included via gdisphw.h to add further win32 specific switches.
   Revision date:     13-04-16

   Version 5.3
   Copyright RAMTEX International ApS 2001-2016

************************************************************************/
#ifndef GHW_PCSIM
#error Only include this file in Gdisp simulation mode
#endif

#ifndef _WIN32
#error Only include this file in WIN32 projects
#endif

/*#define RX_TIMEOUT 5000 */ /* Define to use receive timeout timeout. Default 5 sec (5000 ms). (Only recommended with unreliable communication lines) */
                             /* Commented out --> "No timeout" (default). Rx always waits until all data has been received (or socket connection is closed).*/
                             /* It is recommended to use "No timeout" during debugging. */
#include <stdio.h>
#include <winsock.h>
#include <time.h>
#include <tchar.h>

#include <gdisphw.h>  /* GHW_MINIMIZE_CONSOLE definition */
#include <gsimintf.h>

/* Clean up a illegal RX timeout configuration */
#ifdef RX_TIMEOUT
 #if (RX_TIMEOUT <= 0)
   #undef RX_TIMEOUT  /* Illegal time, use no timeout as default */
 #endif
#endif

/***** Definition of private communication structures used by the LCD simulator    *****/
/***** All knowledge about LCD server communication is encapsulated in this module *****/

/* Define "structure" elements offsets in the communication buffers
  (ie. define "structure" layout in a compiler and processor endian independent way) */

#define  HDR_LEN  0   /* Buffer index for package data length (after header) */
#define  HDR_CMD  2   /* Buffer index for command ID = type of data */
#define  HDR_DAT  4   /* Buffer index for start of data */

/* LCD screen size init data */
#define  DAT_LCDSIZE_W   (HDR_DAT + 0)    /* LCD Screen Width */
#define  DAT_LCDSIZE_H   (HDR_DAT + 2)    /* LCD Screen Height */
#define  DAT_LCDSIZE_P   (HDR_DAT + 4)    /* LCD Screen Pixel mode */
#define  DAT_LCDSIZE     (HDR_DAT + 6)    /* Size of data package */

/* LCD screen area update data */
#define  DAT_LCDAREA_X1   (HDR_DAT + 0)   /* LCD top,left screen area  x */
#define  DAT_LCDAREA_Y1   (HDR_DAT + 2)   /* LCD top,left screen area  y */
#define  DAT_LCDAREA_X2   (HDR_DAT + 4)   /* LCD bottom,right screen area  x */
#define  DAT_LCDAREA_Y2   (HDR_DAT + 6)   /* LCD bottom,right screen area  y */
#define  DAT_LCDAREA_DAT  (HDR_DAT + 8)   /* LCD data start */
#define  DAT_LCDAREA_SIZE DAT_LCDAREA_DAT /* size of package */

#define  GSIM_TOUCH_INFO_SIZE 20  /* RX size for extended touch structure data */

/* Simulator commands use <LOW>..<HIGH> representation (= 80x86 endian)
   These macros assures header data is packed for an endian independent way with no alignment padding */
#define  CTRL_RD(buf,idx)      ((unsigned short)((buf)[(idx)]) + (((unsigned short)((buf)[(idx)+1]))<<8))
#define  CTRL_WR(buf,idx,dat)  (buf)[(idx)] = (unsigned char)((dat)&0xff), (buf)[(idx)+1] = (unsigned char)(((dat)>>8)&0xff)

#define MAX_INFO_LENGTH 0xfffc

/* LCD commands (Note: to assure backward compatibility then commands must only be added to the end) */
typedef enum
   {
   /* Commands requiring LCDserv version 1.1 and later */
   LCD_SIZE = 0x100,    /* LCD (re-size) L_SIZE (set black & white mode) (LCD_READY response) */
   LCD_READY,           /* READY for next command. (default hand-shake acknowledge response from server) */
   LCD_CLR,             /* (Deprecated. Now using LCD_CLR_SYNC)      */
   LCD_INFO_CLR,        /* (Deprecated. Now using LCD_INFO_CLR_SYNC) */
   LCD_BUF,             /* (Deprecated. Now using LCD_BUF_SYNC)      */
   LCD_BUF_SYNC,        /* LCD graphic data buffer L_BUF (LCD_READY response) */
   LCD_INFO,            /* (Deprecated. Now using LCD_INFO_SYNC)     */
   LCD_GETKEY,          /* Get key command (LCD_KEY + data response) */
   LCD_KEY,             /* Key (response on LCD_GETKEY) */
   LCD_SIZE_COLOR,      /* LCD (re-size) L_SIZE and set color (LCD_READY response) */
   LCD_PALETTE,         /* Load LCD palette  (LCD_READY response) */
   LCD_GETTOUCH,        /* Get touch command (LCD_TOUCH + data  response) */
   LCD_TOUCH,           /* Touch (response) */

   /* Commands supported by LCDserv version 3.0 and later */
   LCD_INFO_SYNC=0x110, /* LCD_INFO send C string to info window. LCD_READY response. UTF8 string support */
   LCD_INFO_CLR_SYNC,   /* Like LCD_INFO_CLR  clear info window. Use command synchronization via LCD_READY response */
   LCD_CLR_SYNC,        /* Like LCD_CLR  but with command synchronization via LCD_READY response */
   LCD_FLUSH,           /* Fast time syncronize PC screen with application (for finer animation speed control) */
   LCD_GET_STATUS,      /* Get combined (touch) status info (response LCD_GET_STATUS + data) */
   LCD_GETVERSION,      /* Get LcdServer interface version  (response LCD_GETVERSION + data) */
   LCD_MTOUCH_SIMKEYS,  /* Set touch emulation mode and key scan codes used by multi-touch key simulation (1-4 keyscan codes) (LCD_READY response) */

   LCD_GETTOUCH_1,      /* Get single touch command, return extended multi-touch format (+selects single touch mode)  */
   LCD_GETTOUCH_2,      /* Get two multi-touch command (+select multi touch scan mode)  */
   LCD_GETTOUCH_3,      /* Get three multi-touch command (+select multi touch scan mode) */
   LCD_GETTOUCH_4,      /* Get four multi-touch command (+select multi touch scan mode)  */
   LCD_GETTOUCH_5       /* Get five multi-touch command (+select multi touch scan mode)  */
   } LCD_COMMAND;

/***************** end communication structure definitions *******************/

/***************** Private data *******************/
/*
   The default socket address setup parameters for the LCDSERV server are:
      Name_or_IP : localhost
      Port       : 3000

   If another socket address is wanted the two address definitions
   GSimServer and  GSimPort  below should be modified accordingly.

   When the socket address setup is modified from the default then
   LCDSERV.EXE must be started with three command line parameters:
           0  address_name  port
   similar to:
      LCDSERV.EXE 0 localhost 3000
   or
      LCDSERV.EXE 0 127.0.0.1 3000
*/
static const char *GSimServer = "localhost";
static unsigned short GSimPort = 3000;

/*  */
static SOCKET GSimSocket = INVALID_SOCKET;
static unsigned char *txbuf = NULL;  /* communication buffer */
static unsigned char *lcdbuf = NULL; /* LCD screen copy */
static unsigned short lcdw = 0;      /* LCD screen pixel size (initiate defaults) */
static unsigned short lcdh = 0;
static unsigned short lcdbw = 0;     /* LCD screen byte size */
static GSIM_PALETTE_MODE lcdp = PALETTE_NONE;
static unsigned short pixelmsk;
static unsigned short ilty = 0;      /* "Dirty area" pixel coordinates */
static unsigned short iltx = 0;
static unsigned short irbx = 0;
static unsigned short irby = 0;
static unsigned short oldkey = 0;
static unsigned char  GSimErr = 1;

#define BUFSIZE 80                      /* buffer size for console window titles */
static _TCHAR console_title[BUFSIZE];   /* hold console window title (application name) */
static HWND console_hwnd;               /* handle to console window */


/* Helper macro for displaying errors */
#define PRINTERROR(s)   \
   { \
   if (console_hwnd) \
      ShowWindow( console_hwnd, SW_RESTORE ); /* Restore console window */ \
   fprintf(stderr,"\n%s",(s)); \
   Sleep(2000); /* Show message in at least 2 seconds */ \
   }

/***************** Private functions *******************/

/*
   This function finds the handle to the console window so it can be
   minimized automatically.
   The trick used is to give the console window a unique name temporarily
   so we can use FindWindow() to find it and get its handle for further use.
*/
static void get_console_hwnd(void)
   {
   #ifdef GHW_MINIMIZE_CONSOLE
   _TCHAR NewWindowTitle[BUFSIZE];                /* contains fabricated WindowTitle */
   GetConsoleTitle(console_title, BUFSIZE-1);     /* fetch current window title */
   wsprintf(NewWindowTitle,_T("%d/%d"),           /* format a "unique" NewWindowTitle */
      GetTickCount(),GetCurrentProcessId());
   SetConsoleTitle(NewWindowTitle);               /* Temp change current window title */
   Sleep(40);                                     /* ensure window title has been updated */
   console_hwnd=FindWindow(NULL, NewWindowTitle); /* look for NewWindowTitle */
   SetConsoleTitle(console_title);                /* restore original window title */

   #ifdef _UNICODE
      {
      /* MSVC wide-char compilation mode used, compress info string to a char string for simulator */
      _TCHAR *tcp;
      unsigned char *cp;
      for(tcp = &console_title[0], cp = (unsigned char *) (&console_title[0]); (*tcp & 0xff) != 0; cp++, tcp++)
         {
         if ((*tcp) >= ((_TCHAR) 0xfe))
            { /* Extended file names are not supported */
            strcpy((char *)console_title,"LCD application");
            return;
            }
         *cp = (unsigned char) (*tcp);
         }
      *cp = 0;
      }
   #endif
   #else
   strcpy((char *)console_title,"LCD application");
   console_hwnd = 0;
   #endif
   }


/*
   Receive of data package, check of package length and returned acknowledge response.

   #ifdef RX_TIMEOUT
     Use non-blocking receive of data package, Get timeout on no response from server.
   #else
     Use simple receive of data package (let application wait until either all data is received or socket closed)

   (From version 3.0 command level handshake is always used. The protocol require that we at least
   always receive a LCD_READY to assure that LCD PC application and LCD server screen look is synchronized)
*/
static int recv_buf(unsigned short rcmd, unsigned short rbuflen, unsigned char *rbuf)
   {
   static char hdr[HDR_DAT];
   int cnt;
   /* Check parameters, setup defaults */
   if ((rbuflen==0) || (txbuf == NULL))
      { /* Default handling. Assume reception of simple acknowledge resonce */
      rbuflen = 0;
      rbuf = &hdr[0];  /* Temp buffer for default acknowledge resonse */
      }
   else
      {
      if (rbuf == NULL)
         rbuf = txbuf; /* use txbuf also as default rxbuffer */
      }

   /* Make some values to be overwritten by server data (for command response sanity check) */
   CTRL_WR(rbuf,HDR_LEN,0xFFFF);
   CTRL_WR(rbuf,HDR_CMD,0);
   rbuflen+=4;

   #ifndef RX_TIMEOUT

   /* Use simple RX, wait until all data received (or socket is closed) */
   cnt = recv(GSimSocket, (char *) rbuf, (int) rbuflen, 0);
   /* Check received package size and (acknowledge) response */
   if (cnt == rbuflen)
      { /* Correct number of bytes, check acknowledge command */
      if (CTRL_RD(rbuf, HDR_CMD) == rcmd)
         return 0; /* OK, normal exit */
      }
   PRINTERROR( (cnt > 0) ? "\nERROR during server communication, illegal response" :
              ((cnt== 0) ? "\nERROR during server communication, Socket has been closed."
                         : "\nERROR during server communication"));
   return -1;  /* Do a fatal exit */

   #else  /* RX_TIMEOUT */

   for(;;)  /* Receive loop with Rx timeout management */
      {
      int nRet;
      fd_set fdCheck;
      struct timeval stv;

      /* Set time out val */
      stv.tv_sec  =  (RX_TIMEOUT/1000);
      stv.tv_usec = ((RX_TIMEOUT%1000)*1000);
      /* Set rx timeout and test connection */
      FD_ZERO(&fdCheck);
      FD_SET(GSimSocket, &fdCheck);
      nRet = select(0, &fdCheck, 0, 0, &stv);   /* Enable non-blocking RX operation with timeout */
      if (nRet < 0)
         {
         PRINTERROR("\nERROR: Socket has been closed.");
         return -1;
         }
      if (nRet == 0)
         {
         /* Timeout. No communication response.
            Do complete communication re-init and retry at higher command level */
         return 1;
         }
      if (FD_ISSET(GSimSocket, &fdCheck))
         {
         /* Socket rx ready, collect all data */
         cnt = recv(GSimSocket, (char *) rbuf, (int) rbuflen, 0);
         /* Check received package size and (acknowledge) response */
         if (cnt == rbuflen)
            { /* Correct number of bytes, check acknowledge */
            if (CTRL_RD(rbuf, HDR_CMD) == rcmd)
               return 0; /* OK, normal exit */
            }
         PRINTERROR( (cnt > 0) ? "\nERROR during server communication, illegal response" :
                    ((cnt== 0) ? "\nERROR during server communication, Socket has been closed."
                               : "\nERROR during server communication"));
         return -1;  /* Do a fatal exit */
         }
      /* else Loop until rx ready or timeout */
      }

   #endif /* RX_TIMEOUT */
   }

/*
   Initialize connection to LCD simulator server.
   The simulator server is supposed to be up running already.

   First time init clears LCDserv screen buffer and info buffer
*/
static unsigned char socket_init(void)
   {
   int nRet;
   int firstinit;
   WSADATA wsaData;
   WORD wVersionRequested;
   LPHOSTENT lpHostEntry;
   SOCKADDR_IN saServer;

   GSimErr = 1;
   if (GSimSocket != INVALID_SOCKET)
      {
      closesocket(GSimSocket);
      GSimSocket = INVALID_SOCKET;
      firstinit = 0;
      }
   else
      firstinit = 1;

   /* Initialize WinSock and check the version */
   wVersionRequested = MAKEWORD(1,1);
   WSAStartup(wVersionRequested, &wsaData);
   if (wsaData.wVersion < wVersionRequested)
      {
      fprintf(stderr,"\nERROR Wrong WinSock version\n");
      return 1;
      }

   /* Find the server */
   if ((lpHostEntry = gethostbyname(GSimServer)) == NULL)
      {
      PRINTERROR("ERROR Could not locate server\n LCDSERV.EXE must be up running on this machine in advance");
      return 1;
      }

   /* Create a TCP/IP stream socket */
   if((GSimSocket = socket(PF_INET,          /* Address family */
                         SOCK_STREAM,        /* Socket type */
                         IPPROTO_TCP))       /* Protocol */
       == INVALID_SOCKET)
      {
      PRINTERROR("ERROR socket creation");
      return 1;
      }

   /* Fill in the address structure */
   saServer.sin_family = PF_INET;
   saServer.sin_addr = *((LPIN_ADDR)*lpHostEntry->h_addr_list); /* Server's address */
   saServer.sin_port = htons(GSimPort);                         /* Port number */

   /* connect to the server */
   nRet = connect(GSimSocket,               /* Socket */
                 (LPSOCKADDR)&saServer,    /* Server address */
                  sizeof(struct sockaddr)); /* Length of server address structure */
   if (nRet == SOCKET_ERROR)
      {
      PRINTERROR("ERROR connecting to LCD simulator failed\n LCDSERV.EXE must be up running on this machine");
      closesocket(GSimSocket);
      GSimSocket = INVALID_SOCKET;
      return 1;
      }

   /* Wait for ready response from server */
   if (recv_buf(LCD_READY,0,NULL))
      { /* Illegal Acknowledge reception, socket error or timeout*/
      PRINTERROR("ERROR connecting to LCD simulator failed\n LCDSERV.EXE must be up running on this machine");
      closesocket(GSimSocket);
      GSimSocket = INVALID_SOCKET;
      return 1;
      }

   GSimErr = 0; /* Socket connection ok here */
   if (firstinit)
      {
      /* Prepare console size and screen look */
      get_console_hwnd(); /* Get handle so we can minimize window */
      if (console_hwnd)
         ShowWindow( console_hwnd, SW_MINIMIZE );
      if (GSimGClr())    /* Clear LCD screen buffer on server */
         return 1;
      if (GSimPutsClr()) /* Clear Info buffer on server */
         return 1;
      }
   return 0;
   }

/*
   Internal function. Send command and data to simulator and await confirmation or data from server.

   If RX_TIMEOUT is defined (see top of module), then in the case of no response from server (lost communication)
   a communication re-init is attempted if possible, before returning an error state.

   (Note: If a rbuf is supplied it must always include space for the 4 bytes response header)
*/
static unsigned char GSimTxRx(
   unsigned short tcmd, unsigned short tbuflen, const unsigned char *tbuf,  /* Transmission package(s) */
   unsigned short rcmd, unsigned short rbuflen, unsigned char *rbuf)        /* Reception package (optional) */
   {
   static char hdr[HDR_DAT];  /* Local buffer for command only tx */
   unsigned char *tp;
   int nRet, txlen;
   #ifdef RX_TIMEOUT
   unsigned char retry;
   #endif

   if ((GSimErr == 1) || (GSimSocket == INVALID_SOCKET))
      return 1; /* No init, or permanet error */

   #ifdef RX_TIMEOUT
   for(retry = 2;;)  /* Tx,Rx retry loop */
      {
   #endif
      GSimErr = 1;
      txlen = tbuf ? HDR_DAT : HDR_DAT + tbuflen; /* Use header + foreign buffer, or header (incl buffer) */
      tp = (tbuflen == 0) ? &hdr[0] : txbuf;        /* Use header only or header in buffer */

      /* Init header */
      CTRL_WR(tp,HDR_LEN,tbuflen);
      CTRL_WR(tp,HDR_CMD,tcmd);
      /* Transmit header only or header (in buffer) */
      nRet = send(GSimSocket, (const char *) tp, txlen, 0);
      if (nRet < 0) /* SOCKET_ERROR */
         {
         PRINTERROR("ERROR socket send");
         return 1;
         }
      if (nRet != txlen )
         {
         PRINTERROR("ERROR socket in number of bytes send");
         return 1;
         }
      /* Header send ok */
      if ((tbuf != NULL) && (tbuflen != 0))
         {
         /* Transmit any foreign buffer */
         nRet = send(GSimSocket, (const char *) tbuf, tbuflen, 0);
         if (nRet == SOCKET_ERROR)
            {
            PRINTERROR("ERROR socket send");
            return 1;
            }
         if (nRet != tbuflen )
            {
            PRINTERROR("ERROR socket in number of bytes send");
            return 1;
            }
         /* Foreign buffer transmitted */
         }

      /* Tx completed ok */

      /* Await command synchronization acknowledge, or data */
      nRet = recv_buf(rcmd, rbuflen, rbuf);
      if (nRet == 0)
         {
         GSimErr = 0;
         return 0;  /* Ok, normal exit */
         }
   #ifndef RX_TIMEOUT
      return 1;    /* Fatal rx error (reason already reported) */
   #else
      if (nRet < 0)
         return 1; /* Fatal rx error (reason already reported) */

      /* Try communication error recovery */
      if (retry-- == 0)
         {
         PRINTERROR("\nGiving up retries. Communication lost");
         return 1;   /* Tried twice already, giving up */
         }

      /* Rx timeout, retry with communication recovery */
      PRINTERROR("\nWarning: Communication lost. Doing re-init");
      if (socket_init())
         return 1;  /* Try reconnect failed, give up */

      /* re-connect ok, loop and resend package */
      }
   #endif
   }

/***************** Public functions *******************/

/*
  Init connection to simulator for a LCD display of w*h pixels
     Return ==0 if no errors
     Return !=0 if errors
*/
unsigned char GSimInitC(unsigned short w, unsigned short h, GSIM_PALETTE_MODE p)
   {
   unsigned long lcdbufsize;

   oldkey = 0;
   if (GSimSocket != INVALID_SOCKET)
      {
      /* A LCD simulator connection already exist */
      if ((lcdw == w) && (lcdh == h) && (lcdp == p))
         {
         /* The LCD size and palette is the same so just clear the content */
         if ((GSimGClr() == 0) && (GSimPutsClr() == 0))
            {  /* Communication was ok */
            char str[BUFSIZE+15];
            sprintf(str,"%s re-init.", (char *) console_title);
            return GSimPuts(str);
            }
         }
      GSimClose(); /* Different size or communication failed, so clear and retry */
      }

   /* Create socket connection */
   if (socket_init())
      return 1;

   /* Prepare server window */
   get_console_hwnd(); /* Get handle so we can minimize window */
   if (console_hwnd)
      ShowWindow( console_hwnd, SW_MINIMIZE );

   for(;;)
      {
      /* Server should now be preparred for resize */
      /* Allocate lcd buffer */
      if (lcdbuf != NULL)
         free(lcdbuf);
      lcdbufsize = 0L;
      lcdp = p;
      if (lcdp == PALETTE_NONE)
         {
         lcdbw = (unsigned short)((w+7)/8);
         pixelmsk = (unsigned short) ~(1);
         }
      else
         {
         if (lcdp == PALETTE_RGB)
            lcdbw = (unsigned short)(w * 3);
         else
            lcdbw = (unsigned short)(((((unsigned int) w) *((unsigned int)lcdp))+7)/8);
         /* Convert PALETTE type to a mask value */
         if (lcdp <= PALETTE_1)
            pixelmsk = (unsigned short)~(7); /* 8 pixel in byte */
         else
         if (lcdp <= PALETTE_2)
            pixelmsk = (unsigned short)~(3); /* 4 pixel in byte */
         else
         if (lcdp <= PALETTE_4)
            pixelmsk = (unsigned short)~(1); /* 2 pixel in byte */
         else
            pixelmsk = (unsigned short)~(0); /* 1 pixel in byte */
         }
      lcdw = w;
      lcdh = h;
      if ((lcdbuf = (unsigned char *) calloc(lcdbw,lcdh)) == NULL)
         {
         PRINTERROR("ERROR during buffer allocation");
         break;
         }

      /* Make "dirty-area" rectangle empty */
      iltx = 1;
      ilty = 1;
      irbx = 0;
      irby = 0;

      /* Allocate transmission buffer of optimized size */
      if (txbuf != NULL)
         free(txbuf);

      if (lcdp == PALETTE_RGB)
         lcdbufsize = ((unsigned long) lcdw)*((unsigned long)lcdh)*4;
      else
         lcdbufsize = (unsigned long)(lcdbw*lcdh);
      lcdbufsize += DAT_LCDAREA_SIZE;
      if (lcdbufsize > MAX_INFO_LENGTH)
         lcdbufsize = MAX_INFO_LENGTH+HDR_DAT;

      if ((txbuf = (unsigned char *) malloc(lcdbufsize)) == NULL)
         {
         PRINTERROR("ERROR during buffer allocation");
         break;
         }

      /* Set/update LCD screen size, and color mode */
      CTRL_WR(txbuf,DAT_LCDSIZE_W,w);
      CTRL_WR(txbuf,DAT_LCDSIZE_H,h);
      CTRL_WR(txbuf,DAT_LCDSIZE_P,p);

      if (GSimTxRx(LCD_SIZE_COLOR, DAT_LCDSIZE, NULL, LCD_READY,0,NULL) == 0)
         {
         char str[BUFSIZE+15];
         sprintf(str,"\r%s connected.", (char *) console_title);
         GSimPuts(str);
         return 0; /* Normal exit */
         }
      break;
      }

   /* Some fatal error */
   GSimClose();
   GSimErr = 1;
   return 1;
   }

/*
   Close connection to LCD display simulator by this program.
   If this program is just terminated the connection will be closed automatically.
*/
void GSimClose(void)
   {
   /* Release buffers */
   if (lcdbuf)
      free(lcdbuf);
   lcdbuf = NULL;
   if (txbuf)
      free(txbuf);
   txbuf = NULL;
   lcdw = lcdh = 0;
   /* Close connection */
   if (GSimSocket != INVALID_SOCKET)
      {
      closesocket(GSimSocket);
      GSimSocket = INVALID_SOCKET;
      /* Release WinSock */
      WSACleanup();
      }

   console_hwnd = 0;
   oldkey = 0;
   }

/*
   Return the current error state
*/
unsigned char GSimError(void)
   {
   return GSimErr;
   }

/*
   Clear LCD simulator info window and the keyboard and touch queues

   Return ==0 if no errors
   Return !=0 if errors (no initialization or communication lost)
*/
unsigned char GSimPutsClr(void)
   {
   return GSimTxRx(LCD_INFO_CLR_SYNC, 0, NULL, LCD_READY,0, NULL);
   }

/*
   Transmit an information string to the LCD simulator info window
   The string must be a /NUL (0) terminated C string.
   Supports extended character sets via UFT-8 encoded C strings

   Return ==0 if no errors
   Return !=0 if errors (illegal parameter, no initialization or communication lost)
*/
unsigned char GSimPuts(const char *str)
   {
   unsigned int size;
   if ((str == NULL) || (*str == 0))
      return 0;
   if ((size = strlen(str)) > MAX_INFO_LENGTH) size = MAX_INFO_LENGTH;
   return GSimTxRx(LCD_INFO_SYNC, (unsigned short) (size+1), (const unsigned char *)str,LCD_READY,0,NULL);
   }

/*
   Clear LCD simulator Graphic Window

   Return ==0 if no errors
   Return !=0 if errors (no initialization or communication lost)
*/
unsigned char GSimGClr(void)
   {
   return GSimTxRx(LCD_CLR_SYNC, 0, NULL, LCD_READY,0,NULL);
   }

/*
   Graphic data is transferred bit wise to the LCD buffer so the simulator
   interface is independent of any particular byte orientation in the LCD
   controller.

   The "dirty" area of the buffer must later be flushed to the LCD simulator

   Returns 1 if parameter errors or simulator is uninitialized
   Returns 0 if no errors.
*/
unsigned char GSimWrBit(unsigned short x, unsigned short y, unsigned char val)
   {
   static const unsigned char mask[]  = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};
   static const unsigned char mask2[] = {0xC0,0x30,0x0C,0x03};
   static const unsigned char mask4[] = {0xF0,0x0F};

   if ((lcdbuf == NULL) || (x >= lcdw) || (y >= lcdh))
      {
      GSimErr = 1;
      return 1;  /* Parameter error or missing initialization */
      }

   switch (lcdp)
      {
      case PALETTE_NONE:
      case PALETTE_1:
         if ((val & 0x1) != 0)
            lcdbuf[x/8+lcdbw*y] |= mask[x & 0x7];
         else
            lcdbuf[x/8+lcdbw*y] &= ~mask[x & 0x7];
         break;
      case PALETTE_2:
         val = (unsigned char)((val & 0x3) << (6-(x & 0x3)*2));
         lcdbuf[x/4+lcdbw*y] = (unsigned char)((lcdbuf[x/4+lcdbw*y] & ~mask2[x & 0x3]) | val);
         break;
      case PALETTE_4:
         val &= 0xf;
         if ((x & 0x1) == 0)
            val <<= 4;
         lcdbuf[x/2+lcdbw*y] = (unsigned char)((lcdbuf[x/2+lcdbw*y] & ~mask4[x & 0x1]) | val);
         break;
      case PALETTE_8:
         lcdbuf[x+lcdbw*y] = val;
         break;
      default:
         return 1; /* Illegal function for mode */
      }

   /* Update "dirty area" markers */
   if(  irbx < iltx )
       iltx = irbx = x;
   else if( x < iltx )
       iltx = x;
   else if( x > irbx )
       irbx = x;

   if( irby < ilty)
       ilty = irby = y;
   else if( y < ilty )
       ilty = y;
   else if( y > irby )
       irby = y;
   return 0;
   }

/*
   Graphic RGB data is transferred pixel wise to the LCD buffer so the simulator
   interface is independent of any particular byte orientation in the LCD controller.

   The "dirty" area of the buffer must later be flushed to the LCD simulator

   Returns 1 if parameter errors or simulator is uninitialized
   Returns 0 if no errors.
*/
unsigned char GSimWrRGBBit(unsigned short x, unsigned short y, GSIM_RGB_PARAM val)
   {
   unsigned char *bufp;
   if ((lcdbuf == NULL) || (x >= lcdw) || (y >= lcdh) || lcdp != PALETTE_RGB)
      {
      GSimErr = 1;
      return 1;  /* Parameter error or missing initialization */
      }

   bufp = &lcdbuf[x*3+lcdbw*y];
   *bufp++ = val.rgb.r;
   *bufp++ = val.rgb.g;
   *bufp++ = val.rgb.b;

   /* Update "dirty area" markers */
   if( irbx < iltx )
       iltx = irbx = x;
   else if( x < iltx )
       iltx = x;
   else if( x > irbx )
       irbx = x;

   if( irby < ilty)
       ilty = irby = y;
   else if( y < ilty )
       ilty = y;
   else if( y > irby )
       irby = y;
   return 0;
   }

/*
   Update palette data.
   The number of palette elements must match the palette mode
*/
unsigned char GSimWrPalette(const GSIM_PALETTE_RGB *palette, unsigned short no_elements)
   {
   unsigned short i;
   unsigned char *cp;
   if(lcdbuf == NULL)
      {
      GSimErr = 1;
      return 1;
      }

   if ((lcdp != PALETTE_1) &&
       (lcdp != PALETTE_2) &&
       (lcdp != PALETTE_4) &&
       (lcdp != PALETTE_8))
       return 1;    /* Command illegal for mode */

   /* power_of_2(palette_mode) == palette table size */
   if (((unsigned short) ((1<<lcdp))) !=  no_elements)
       return 1;    /* Palette size is not correct for mode */

   /* Load palette to transmission buffer */
   cp = &txbuf[HDR_DAT];
   for (i = 0; i < no_elements; i++, palette++)
      {
      *cp++ = palette->r;
      *cp++ = palette->g;
      *cp++ = palette->b;
      }

   return GSimTxRx(LCD_PALETTE, (unsigned short) (i*3), NULL, LCD_READY,0,NULL);
   }

/*
   Flush "dirty" part of graphic buffer to the LCD simulator

   Return 1 if simulator is uninitialized or communication errors
   Return 0 if no errors.
*/
unsigned char GSimFlush(void)
   {
   unsigned short x,ilx,irx;
   unsigned char *txp;
   unsigned long i;
   if ((lcdbuf == NULL) || (txbuf == NULL))
      {
      GSimErr = 1; /* Missing initialization */
      return 1;
      }
   if((irby < ilty ) || ( irbx < iltx ))
      return 0; /* No new data to flush */

   /* Align update retangle to horizontal byte limits */
   iltx &= pixelmsk;   /* Start from a whole buffer byte */

   do
      {
      /* Copy coordinates to buffer header */
      CTRL_WR(txbuf, DAT_LCDAREA_X1, iltx);
      CTRL_WR(txbuf, DAT_LCDAREA_Y1, ilty);
      CTRL_WR(txbuf, DAT_LCDAREA_X2, irbx);
      CTRL_WR(txbuf, DAT_LCDAREA_Y2, irby);

      /* Copy dirty area data to transmission buffer */
      i=0;
      txp = &txbuf[DAT_LCDAREA_DAT];

      if (lcdp == PALETTE_RGB)
         {
         unsigned char r,g,b;
         unsigned char *bufp;
         /* Save dirty area using color counting / compression */
         for (; ilty <= irby; ilty++)
            {
            if ((i + ((irbx-iltx)+1)*4 + DAT_LCDAREA_DAT+1) > MAX_INFO_LENGTH)
               {
               register unsigned short y2;
               y2 = (unsigned short)((ilty != 0) ? ilty-1 : 0);
               CTRL_WR(txbuf, DAT_LCDAREA_Y2, y2);
               break;  /* Buffer length type limit reached, use segmented transmit */
               }
            bufp = &lcdbuf[iltx*3+ilty*lcdbw];
            for (x = iltx; x <= irbx; x++)
               {
               r = *bufp++;
               g = *bufp++;
               b = *bufp++;
               if (iltx == x)
                  {
                  /* First pixel on line, just save it */
                  i+=4;
                  txp[0] = r;
                  txp[1] = g;
                  txp[2] = b;
                  txp[3] = 0;
                  }
               else
                  {
                  /* Check against previous pixel */
                  if ((txp[0] == r) &&
                      (txp[1] == g) &&
                      (txp[2] == b) &&
                      (txp[3] < 255))
                     {
                     txp[3]++;   /* The color is the same, just increment count */
                     }
                  else
                     {
                     /* Use a new position */
                     txp = &txp[4];
                     i+=4;
                     txp[0] = r;
                     txp[1] = g;
                     txp[2] = b;
                     txp[3] = 0;
                     }
                  }
               }
            txp = &txp[4]; /* Use a new position for next line */
            }
         }
      else
         {
         /* Convert to whole byte coordinates */
         if (lcdp == PALETTE_NONE)
            {
            ilx = (unsigned short)(iltx / 8);
            irx = (unsigned short)(irbx / 8);
            }
         else
            {
            ilx = (unsigned short)(iltx / (8/(unsigned short) lcdp));
            irx = (unsigned short)(irbx / (8/(unsigned short) lcdp));
            }
         for (; ilty <= irby; ilty++)
            {
            if ((i + (irx-ilx)+1) >  MAX_INFO_LENGTH)
               {
               register unsigned short y2;
               y2 = (unsigned short)((ilty != 0) ? ilty-1 : 0);
               CTRL_WR(txbuf, DAT_LCDAREA_Y2, y2);
               break;  /* Buffer length type limit reached, use segmented transmit */
               }

            for (x = ilx; x <= irx; x++)
               {
               *txp++ = lcdbuf[x+ilty*lcdbw];
               i++;
               }
            }
         }

      /* transmit data and wait for response */
      if (GSimTxRx(LCD_BUF_SYNC, (unsigned short)(DAT_LCDAREA_DAT + i), NULL, LCD_READY,0, NULL))
         break; /* Some communication error (GSimErr = 1) */
      }
   while( ilty <= irby );

   /* Clear dirty area */
   iltx = 1;
   ilty = 1;
   irbx = 0;
   irby = 0;
   return GSimErr;
   }

/*
   GSimSync() assure that any buffered data in the communication pipelines has been processed
   and made visible on the LCDserv.exe PC screen.

   Its purpose is to give fast animations some finer grained timing control the over the server "screen look".
   On "localhost" it can assure client/server screen states are syncronized with a timing screw < 20ms.
   Usually GSimSync() is just called once after a serie of GSimFlush() calls in a complex drawing session.

   Return 1 if communication errors
   Return 0 if no errors.
*/
unsigned char GSimSync(void)
   {
   return GSimTxRx(LCD_FLUSH, 0, NULL, LCD_FLUSH, 0, NULL);
   }

/*
   Simulates a 10ms timerstamp counter
*/
unsigned long GSimTimestamp(void)
   {
   return (((unsigned long) clock())*100L)/CLOCKS_PER_SEC;
   }

/***************************** Commands fetching data from simulator *********************/

/* Macros to make RX format independent of compiler endian and structure padding */
/* For use of use data in <HIGH>-<LSB> order (Similar to many touch screen controllers) */
#define  RD8(buf,idx)    ((unsigned char)buf[(idx)])
#define  RD16(buf,idx) ((((unsigned short)buf[(idx)])<<8) | ((unsigned short)buf[(idx)+1]) )
#define  RD32(buf,idx) ((((unsigned long)buf[(idx)])<<24) | (((unsigned long)buf[(idx)+1])<<16) | (((unsigned long)buf[(idx)+2])<<8) | (((unsigned long)buf[(idx)+3])))
#define  WR16(buf,idx,dat) ((unsigned char *)(buf))[(idx)+1] = (unsigned char)((dat)&0xff), ((unsigned char *)(buf))[(idx)] = (unsigned char)(((dat)>>8)&0xff)

/*
   Give more processing time to other applications if using a tight poll loop
   Called from status poll functions if no change is detected
*/
static void wait_delay(void)
   {
   static char poll_delay_cnt = 0;
   if ((++poll_delay_cnt % 10) == 0)
      Sleep(5);
   }

/*
   Get command response word, check return response and return a short data word.
   Local function.
*/
static int get_cmd_word(unsigned short tcmd, unsigned short rcmd )
   {
   if (GSimTxRx(tcmd, 0, NULL, rcmd, 2, NULL))
      return -1;
   return (int)((unsigned int)RD16(txbuf,HDR_DAT));
   }

/*
   Get a key from LCD server
*/
unsigned short GSimKbGet(void)
   {
   int key;
   if ((oldkey != 0) || (GSimErr != 0))
      {
      key = oldkey;
      oldkey = 0;
      return (unsigned short) key;
      }

   /* Get key from server with get_cmd_word(..)
        returns  > 0 if data
        returns == 0 if no new data
        returns  < 0 if socket error */
   while ((key = get_cmd_word( LCD_GETKEY, LCD_KEY )) >= 0)
      {
      if (key != 0)
         return (unsigned short) key;
      Sleep(5);   /* Give more processing time to other applications */
      }
   return 0;
   }

/*
   Check a key from LCD server
*/
unsigned short GSimKbHit(void)
   {
   int key;
   if (GSimErr != 0)
      return 0;
   if (oldkey != 0)
      return 1;
   if ((key = get_cmd_word( LCD_GETKEY, LCD_KEY )) < 0)
      return 0;  /* Some server connection error */
   oldkey = (unsigned short) key;
   return (unsigned short)((oldkey != 0) ? 1 : 0);
   }

/*
   Get combined status flags (combined bit flags)

      Return 0x0001   Simulator key data ready
      Return 0x0002   Touch data ready
      Return 0x8000   Lost connection status

*/
unsigned short GSimStatus(void)
   {
   int status;
   if ((status = get_cmd_word( LCD_GET_STATUS, LCD_GET_STATUS )) < 0)
       status=0x8000;  /* Lost connection error flag */
   if (status == 0)
      wait_delay();    /* Give more processing time to other applications if using a tight poll loop */
   return (unsigned short) ((unsigned int)status);
   }

/****************** Touch Screen simulation interfaces **********************/


/*
   Simulates a touch screen input.
   Returns 1 if a touch change (an edge or pressed coordinate change) has been detected
   Returns 0 if the touch level is unchanged (unpressed, or pressed and same position).

   Edge  = 1, Touch screen pressed changed state.
   Edge  = 0, No touch state change.
   Press = 1, Touch screen pressed (x,y values is a (new) valid position)
   Press = 0, No touch (x,y values is the position where last touch stopped)

   (Old single touch function interface)
*/
unsigned char GSimTouchGet(unsigned char *edgep, unsigned char *pressp, unsigned short *xp, unsigned short *yp)
   {
   static unsigned short oldx = 0xffff;
   static unsigned short oldy = 0xffff;
   unsigned short x,y;
   unsigned char edge;
   if (GSimTxRx(LCD_GETTOUCH, 0, NULL, LCD_TOUCH, 1+1+2+2, NULL))
      return 0;

   edge = RD8(txbuf,HDR_DAT); /* edge */
   if (edgep != NULL)
      *edgep  = edge; /* edge */
   if (pressp != NULL)
      *pressp = RD8(txbuf,HDR_DAT+1); /* press state */
   x = RD16(txbuf,HDR_DAT+2);    /* x */
   y = RD16(txbuf,HDR_DAT+4);    /* y */
   if (xp != NULL)
      *xp = x;
   if (yp != NULL)
      *yp = y;
   if (edge == 0)
           if ((x != oldx) || (y != oldy))
                   edge = 1; /* Position change */
   if (edge == 0) /* no Change ? */
      wait_delay(); /* Give more processing time to other applications if using a tight poll loop */
   return edge;
   }

/*
   Read the current extended touch state info for one or more touch keys

   GSIM_TOUCH_INFO *touch_info            Pointer to array of GSIM_TOUCH_INFO structures
                                          Updates a GSIM_TOUCH_INFO structure if touch_info != NULL
   unsigned char num_multi_touch_keys     Number of touch keys to support and read (must be <= 5 keys).
                                          (0 = default = single touch key)
   unsigned char *num_touch               If != NULL initialized with the number of pressed keys

   Returns 0 if there is no change in any touch state
   Returns 1 if there is some touch state change (touch up, down, drag) in any of the supported touch keys


   GSIM_TOUCH_INFO data description:

      Quick test flags:
         change;  True if edge event or position change detected during touch active state.
                  (= return parameter)
         edge;    edge event:  = 1 if touch down or touch up edge detected.
         press;   Touch state: = 1 if touch is active (pressed down)

      Current touch state:
         x,y      Current position. Range 0,0 - GDISPW-1,GDISPH-1
         t        Time stamp for current touch point change (10 ms resolution)
                  (The time stamp is provided by the LCDserv and should always be
                   used relative to t1, or a previous GSIM_TOUCH_INFO time stamp info)

      Touch down (first) state. (For use as base for relative touch position changes)
         x1,y1    Touch down position = first active position.
         t1       Time stamp for touch down start.

      Example: Touch swipe "speed" calculations (Usually done after touch up edge detected).
         x drag "speed" is  (x-x1)/(t-t1)
         y drag "speed" is  (y-y1)/(t-t1)
         Assumes that touch event duration time = (t-t1) > 0.

*/
unsigned char GSimTouchInfo(GSIM_TOUCH_INFO *touch_info, unsigned char num_multi_touch_keys, unsigned char *pnum_touch )
   {
   unsigned char i, press, change;
   unsigned short cmd;

   if (num_multi_touch_keys > 1)
      {
      if (num_multi_touch_keys > GTOUCH_MAX_PRESSED)
         num_multi_touch_keys = GTOUCH_MAX_PRESSED;
      }
   else
      num_multi_touch_keys = 1;

   /* LCD_GETTOUCH_1 + {0,1,2,3,4} configures the Lcdserv multi-touch interface (1 to 5 touch keys) */
   cmd = LCD_GETTOUCH_1+(num_multi_touch_keys-1);
   if (GSimTxRx(cmd, 0, NULL, cmd, GSIM_TOUCH_INFO_SIZE*num_multi_touch_keys, NULL))
      return 0; /* Communication error */

   /* Process touch data */
   for (i=0, press=0, change=0; i<num_multi_touch_keys; i++)
      {
      register unsigned char *buf = &txbuf[HDR_DAT + i*GSIM_TOUCH_INFO_SIZE];
      if (touch_info != NULL)
         {
         register GSIM_TOUCH_INFO *tinfo = &touch_info[i];
         tinfo->change = RD8(buf,0);
         tinfo->id     = RD8(buf,1);   /* = 0 for single touch, touch tracking ID (1-5) for multi-touch */
         tinfo->edge   = RD8(buf,2);
         tinfo->press  = RD8(buf,3);
         tinfo->x      = RD16(buf,4);
         tinfo->y      = RD16(buf,6);
         tinfo->t      = RD32(buf,8);
         tinfo->x1     = RD16(buf,12);
         tinfo->y1     = RD16(buf,14);
         tinfo->t1     = RD32(buf,16);
         }
      change |= RD8(buf,0); /* = touch_info[i]->change */
      if (RD8(buf,3))       /* = touch_info[i]->press */
         press++;
      }
   if (pnum_touch != NULL)
      *pnum_touch = press;  /* Set number of actively pressed positions */
   if (change == 0)
      wait_delay(); /* Nothing new */ /* Give more processing time to other applications if using a tight poll loop */
   return change;
   }

/*
   Configure LCDserv touch controller emulation

    Touch data management:

     touch_mode = 0 ->     Detailed drag (default, simplest PC mode use).
                           Emulates a touch controller with build-in event fifo.
                           -> All touch up / down events and drag position changes reported
                           Best for fast human drawing operations on (simulated) touch screen.

     touch_mode = 1 ->     Fast drag
                           Emulates a touch controller with interrupt on only touch up / touch down states.
                           All touch up / down events will be reported.
                           Drag positions may "jump" in large steps if not read / polled fast enough by client.
                           Best for fast swipe operations on (simulated) touch screen.

     touch_mode = 2 ->     Touch state only
                           Emulates a touch controller with no fifo and no event interrupt, i.e. which
                           provides the current touch state information only.
                           -> if not read / polled fast enough some touch up/down events may not be detected
                           and drag positions may "jump"
                           Best if application does not depend on "hardware" touch up/down edge detection
                           nor on drag operations

    Change PC keys used by emulated multi-touch:

     num_hold_keys         Number of scan codes in holdkey_scan_codes {0 - GTOUCH_MAX_PRESSED-1}
                           Use 0 if no change to PC keys used for emulating multi-touch

     holdkey_scan_codes[]  Change scan codes for PC keys used by multi-touch emulation. Default=('A','S','D','F')
                           The first key scan code (at index 0) is used with dual touch emulation,
                           Can optionally be used together with <shift> and/or <ctrl> PC keys to emulate dual
                           drag or area zoom touch events.
*/
void GSimTouchSetup(unsigned short touch_mode, unsigned char num_hold_keys, unsigned short *holdkey_scan_codes )
   {
   unsigned char i, len;
   if (holdkey_scan_codes == NULL)
      num_hold_keys = 0;
   else
   if (num_hold_keys > GTOUCH_MAX_PRESSED-1)
      num_hold_keys =  GTOUCH_MAX_PRESSED-1;

   WR16(txbuf, 0, touch_mode);                /* Set new touch emulation mode */
   for (len=2, i=0; i<num_hold_keys;i++)      /* Set new multi-touch_holdkeys */
      {
      WR16(txbuf, len, holdkey_scan_codes[i]);
      len+=2;
      }

   GSimTxRx(LCD_MTOUCH_SIMKEYS, len, txbuf, LCD_READY, 0, NULL);
   }

/*
   Get LCD server command interface version.

   Return data, 4 bit pr digit, 4 digits.
   Example: Return 0x0300 -> version 3.00  (which is the first version supporting this command)

   This function has no impact on internal LCDserv.exe execution states.
   May therefore be used for simple "Is LCD server still connected ?" tests.

   Returns 0 in case of socket error (not connected)
*/
unsigned short GSimVersion(void)
   {
   int version;
   if ((version = get_cmd_word( LCD_GETVERSION, LCD_GETVERSION )) < 0)
       version=0; /* Use 0 as error */
   return (unsigned short) ((unsigned int)version);
   }


