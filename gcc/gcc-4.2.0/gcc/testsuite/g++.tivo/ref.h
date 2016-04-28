/*
 * ref.h
 *
 * Defines both the ref<T> and nonref<T> smart pointer templates.
 * 
 *
 * part of the test suite for the TiVo ref-counting
 * rules added to g++.
 *
 *
 *
 */

#ifndef __REF_H__
#define __REF_H__

#define NULL (0)

template<class T>
class nonRef
{
  public:
    nonRef();
    ~nonRef();
    nonRef( T* );
    nonRef( const nonRef& );
    nonRef<T>& operator=( const nonRef& );
    nonRef<T>& operator=( T* );
    bool operator==( const nonRef& );
    operator T*() const;
    T* operator->() const;

  private:
    T* ptrM;   /* breaks rules if T is subclass of TmkCore
		* but there's a special case in compiler for
		* nonRef<>
		*/
};

template<class T>
class ref
{
  public:
    ref();
    ~ref();
    ref( T* );
    ref( const ref& );
    ref<T>& operator=( const ref& );
    ref<T>& operator=( T* );
    bool operator==( const ref& );
    T* Get() const;
    operator T*() const;
    T* operator->() const;

  private:
    T* ptrM;   /* breaks rules if T is subclass of TmkCore
		* but there's a special case in compiler for
		* ref<>
		*/
};

///////////////////////////////////////////
//
// inline implementatiosn go below here!
//
///////////////////////////////////////////

/////////////////
//
// ref
//
/////////////////

template <class T>
inline ref<T>::ref()
{
    ptrM = NULL;
}

template <class T>
inline ref<T>::ref( const ref<T>& other )
{
    ptrM = other.ptrM;
    if ( ptrM != NULL )
    {
	((TmkCore*)ptrM)->ref();
    }
}

template <class T>
inline ref<T>::ref( T* ptr )
{
    ptrM = ptr;
    if ( ptrM != NULL )
    {
	((TmkCore*)ptrM)->ref();
    }
}

template <class T>
inline ref<T>::~ref()
{
    if ( ptrM != NULL )
    {
	((TmkCore*)ptrM)->unref();
    }
}

template <class T>
inline ref<T>& ref<T>::operator = ( const ref<T>& other )
{
    // Pull the pointer out of other, just in case other is contained
    // in something owned by the previous value, and it goes away when
    // we call unref.
    nonRef<T> ptr = other.ptrM;
    
    // incrementing before decrementing handles assignment-to-self case
    if ( ptr != NULL )
    {
	((TmkCore*)ptr)->ref();
    }
    if ( ptrM != NULL )
    {
	((TmkCore*)ptrM)->unref();
    }
    ptrM = ptr;
    return *this;
}

template <class T>
inline ref<T>& ref<T>::operator = ( T* ptr )
{
    // incrementing before decrementing handles assignment-to-self case
    if ( ptr != NULL )
    {
	((TmkCore*)ptr)->ref();
    }
    if ( ptrM != NULL )
    {
	((TmkCore*)ptrM)->unref();
    }
    ptrM = ptr;
    return *this;
}

template <class T>
inline ref<T>::operator T* () const
{
    return ptrM;
}

template <class T>
inline T* ref<T>::operator -> () const
{
    return ptrM;
}

/////////////////
//
// nonRef
//
/////////////////

template <class T>
inline nonRef<T>::nonRef()
{
    ptrM = NULL;
}

template <class T>
inline nonRef<T>::nonRef( const nonRef<T>& other )
{
    ptrM = other.ptrM;
}

template <class T>
inline nonRef<T>::nonRef( T* ptr )
{
    ptrM = ptr;
}

template <class T>
inline nonRef<T>& nonRef<T>::operator = ( const nonRef<T>& other )
{
    ptrM = other.ptrM;
    return *this;
}

template <class T>
inline nonRef<T>& nonRef<T>::operator = ( T* ptr )
{
    ptrM = ptr;
    return *this;
}

template <class T>
inline nonRef<T>::operator T* () const
{
    return ptrM;
}

template <class T>
inline T* nonRef<T>::operator -> () const
{
    return ptrM;
}

////////
// 
// Other test ref-looking template classes...
// 
////////

template<class T>
class refConst
{
  public:
    refConst();

  private:
    T* ptrM;
};

template <class T>
inline refConst<T>::refConst()
{
    ptrM = NULL;
}

template<class T>
class abRefthis
{
  public:
    abRefthis();

  private:
    T* ptrM;
};

template <class T>
inline abRefthis<T>::abRefthis()
{
    ptrM = NULL;
}

template<class T>
class referenceNot
{
  public:
    referenceNot();

  private:
    T* ptrM;
};

template <class T>
inline referenceNot<T>::referenceNot()
{
    ptrM = NULL;
}
#endif // __REF_H__
