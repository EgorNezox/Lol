/**
  ******************************************************************************
  * @file    qm.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  ******************************************************************************
  */

//TODO: dynamic memory management and verification

#ifndef QM_H_
#define QM_H_

#define QM_UNUSED(x) (void)x;


#define QM_FORWARD_PRIVATE(Class) class Class##Private;

#define QM_DECLARE_PRIVATE(Class) \
    inline Class##Private* d_func() { return reinterpret_cast<Class##Private *>(d_ptr); } \
    inline const Class##Private* d_func() const { return reinterpret_cast<const Class##Private *>(d_ptr); } \
    friend class Class##Private;

#define QM_DECLARE_PUBLIC(Class) \
    inline Class* q_func() { return static_cast<Class *>(q_ptr); } \
    inline const Class* q_func() const { return static_cast<const Class *>(q_ptr); } \
    friend class Class;

#define QM_D(Class) Class##Private * const d = d_func()
#define QM_Q(Class) Class * const q = q_func()


#define QM_DISABLE_COPY(Class) \
    Class(const Class &) = delete;\
    Class &operator=(const Class &) = delete;

#endif /* QM_H_ */
