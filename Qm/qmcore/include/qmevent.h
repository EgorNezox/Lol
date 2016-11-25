/**
  ******************************************************************************
  * @file    qmevent.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  ******************************************************************************
  */

#ifndef QMEVENT_H_
#define QMEVENT_H_

/*! The QmEvent class is the base class of all event classes.
 *  Event objects contain event parameters.
 *
 * Qm's main event loop (QmApplication::exec()) fetches QmEvents
 * from the event queue and sends them to \l{QmObject}s.
 *
 * QmObjects receive events by having their QmObject::event() function
 * called. The function can be reimplemented in subclasses to
 * customize event handling and add additional event types.
 *
 * The basic QmEvent contains only an event type parameter.
 * Subclasses of QEvent contain additional parameters that describe
 * the particular event.
 *
 * \sa QmObject::event(), QmApplication::sendEvent(), QmApplication::postEvent()
 */
class QmEvent {
public:
	/*! This enum type defines the valid event types and the specialized classes for each type
	 *
	 * User events should have values between \c User and \c{MaxUser}.
	 */
	enum Type {
		None = 0,				/*!< not an event */
		DeferredDelete,			/*!< the object will be deleted after it has cleaned up */
		ThreadFinishSync,		/*!< internal synchronization mechanism between QmThread object and thread implementation (platform-specific) */
		Timer,					/*!< internal event used by QmTimer (platform-specific) */
		HardwareIO,				/*!< internal event used by classes in HardwareIO module (platform-specific) */
		KeysInput,				/*!< internal event used by classes in KeysInput module (platform-specific) */
		KeyStateChanged,		/*!< internal event used by classes in KeysInput module (platform-specific) */
		KeyAction,				/*!< internal event used by classes in KeysInput module (platform-specific) */
		RtcWakeup,				/*!< internal event used by classes in RTC module (platform-specific) */
		User = 1000,			/*!< user-defined event */
		MaxUser = 65535 - User	/*!< last user event ID */
	};

	/*! Contructs an event object of type \a type. */
	QmEvent(Type type) : t(type) {};

	/*! Destroys the event. */
	virtual ~QmEvent() {};

	/*! Returns the event type */
	inline Type type() const { return static_cast<Type>(t); }

private:
	unsigned int t;
};

#endif /* QMEVENT_H_ */
