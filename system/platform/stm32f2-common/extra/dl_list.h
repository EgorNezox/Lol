/**
  ******************************************************************************
  * @file    dl_list.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    04.01.2016
  *
  ******************************************************************************
 */

#ifndef DL_LIST_H_
#define DL_LIST_H_

#define DLLIST_TYPEDEF_LIST(element_type, list_id) \
		typedef struct { \
			element_type *head, *tail; \
		} dllist_##list_id;
#define DLLIST_LIST_TYPE(list_id)	dllist_##list_id
#define DLLIST_LIST_POD_INITIALIZER {0, 0}

#define DLLIST_ELEMENT_FIELDS(element_type, list_id) \
		dllist_##list_id *_dll_##list_id; \
		element_type *_dllprev_##list_id, *_dllnext_##list_id;
#define DLLIST_ELEMENT_POD_INITIALIZER 0, 0, 0
#define DLLIST_ELEMENT_CLASS_INITIALIZER(list_id) \
		_dll_##list_id(0), _dllprev_##list_id(0), _dllnext_##list_id(0)

#define DLLIST_INIT_LIST(list_ptr)	\
		do { \
			(list_ptr)->head = 0; \
			(list_ptr)->tail = 0; \
		} while(0)

#define DLLIST_INIT_ELEMENT(list_id, element_ptr) \
		do { \
			(element_ptr)->_dll_##list_id = 0; \
			(element_ptr)->_dllprev_##list_id = 0; \
			(element_ptr)->_dllnext_##list_id = 0; \
		} while(0)

#define DLLIST_IS_IN_LIST(list_id, list_ptr, element_ptr) ((element_ptr)->_dll_##list_id == (list_ptr))

#define DLLIST_IS_LIST_EMPTY(list_ptr) (((list_ptr)->head == 0) && ((list_ptr)->tail == 0))

#define DLLIST_GET_LIST_FRONT(list_ptr) ((list_ptr)->head)

#define DLLIST_GET_LIST_BACK(list_ptr) ((list_ptr)->tail)

#define DLLIST_GET_NEXT(list_id, element_ptr) ((element_ptr)->_dllnext_##list_id)

#define DLLIST_GET_PREV(list_id, element_ptr) ((element_ptr)->_dllprev_##list_id)

#define DLLIST_ADD_TO_LIST_BACK(list_id, list_ptr, element_ptr) \
		do { \
			(element_ptr)->_dll_##list_id = (list_ptr); \
			if (!DLLIST_IS_LIST_EMPTY(list_ptr)) { \
				(element_ptr)->_dllprev_##list_id = (list_ptr)->tail; \
				(list_ptr)->tail->_dllnext_##list_id = (element_ptr); \
				(list_ptr)->tail = (element_ptr); \
			} else { \
				(list_ptr)->tail = (element_ptr); \
				(list_ptr)->head = (element_ptr); \
			} \
		} while(0)

#define DLLIST_ADD_TO_LIST_FRONT(list_id, list_ptr, element_ptr) \
		do { \
			(element_ptr)->_dll_##list_id = (list_ptr); \
			if (!DLLIST_IS_LIST_EMPTY(list_ptr)) { \
				(element_ptr)->_dllnext_##list_id = (list_ptr)->head; \
				(list_ptr)->head->_dllprev_##list_id = (element_ptr); \
				(list_ptr)->head = (element_ptr); \
			} else { \
				(list_ptr)->head = (element_ptr); \
				(list_ptr)->tail = (element_ptr); \
			} \
		} while(0)

#define DLLIST_INSERT_TO_LIST_AFTER(list_id, prev_element_ptr, element_ptr) \
		do { \
			dllist_##list_id *list_ptr = (prev_element_ptr)->_dll_##list_id; \
			(element_ptr)->_dll_##list_id = (list_ptr); \
			if ((list_ptr)->tail == (prev_element_ptr)) { \
				(list_ptr)->tail = (element_ptr); \
			} else { \
				(element_ptr)->_dllnext_##list_id = (prev_element_ptr)->_dllnext_##list_id; \
				(prev_element_ptr)->_dllnext_##list_id ->_dllprev_##list_id = (element_ptr); \
			} \
			(prev_element_ptr)->_dllnext_##list_id = (element_ptr); \
			(element_ptr)->_dllprev_##list_id = (prev_element_ptr); \
		} while(0)

#define DLLIST_INSERT_TO_LIST_BEFORE(list_id, next_element_ptr, element_ptr) \
		do { \
			dllist_##list_id *list_ptr = (next_element_ptr)->_dll_##list_id; \
			(element_ptr)->_dll_##list_id = (list_ptr); \
			if ((list_ptr)->head == (next_element_ptr)) { \
				(list_ptr)->head = (element_ptr); \
			} else { \
				(element_ptr)->_dllprev_##list_id = (next_element_ptr)->_dllprev_##list_id; \
				(next_element_ptr)->_dllprev_##list_id ->_dllnext_##list_id = (element_ptr); \
			} \
			(next_element_ptr)->_dllprev_##list_id = (element_ptr); \
			(element_ptr)->_dllnext_##list_id = (next_element_ptr); \
		} while(0)

#define DLLIST_REMOVE_FROM_LIST(list_id, list_ptr, element_ptr) \
		do { \
			if ((element_ptr)->_dllprev_##list_id) { \
				(element_ptr)->_dllprev_##list_id ->_dllnext_##list_id = (element_ptr)->_dllnext_##list_id; \
			} else { \
				(list_ptr)->head = (element_ptr)->_dllnext_##list_id; \
			} \
			if ((element_ptr)->_dllnext_##list_id) { \
				(element_ptr)->_dllnext_##list_id ->_dllprev_##list_id = (element_ptr)->_dllprev_##list_id; \
			} else { \
				(list_ptr)->tail = (element_ptr)->_dllprev_##list_id; \
			} \
			(element_ptr)->_dll_##list_id = 0; \
			(element_ptr)->_dllprev_##list_id = 0; \
			(element_ptr)->_dllnext_##list_id = 0; \
		} while(0)

#endif /* DL_LIST_H_ */
