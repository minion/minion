/*
* Minion http://minion.sourceforge.net
* Copyright (C) 2006-09
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/*
 * linked_ptr - simple reference linked pointer
 * (like reference counting, just using a linked list of the references

 * instead of their count.)
 *
 * The implementation stores three pointers for every linked_ptr, but

 * does not allocate anything on the free store.
 */

#ifndef LINKED_PTR_H
#define LINKED_PTR_H
template <class X> class shared_ptr
{
public:


#define TEMPLATE_FUNCTION template <class Y>  
TEMPLATE_FUNCTION friend class shared_ptr;

  typedef X element_type;
  explicit shared_ptr(X* p = 0) throw()
        : itsPtr(p) {itsPrev = itsNext = this;}
        
        
    ~shared_ptr()
        {release();}

    shared_ptr(const shared_ptr& r) throw()
        {acquire(r);}

    shared_ptr& operator=(const shared_ptr& r)

    {
        if (this != &r) {
            release();
            acquire(r);
        }
        return *this;
    }


#ifdef PRIV
    template <class Y> friend class shared_ptr;
#endif

    template <class Y> shared_ptr(const shared_ptr<Y>& r) throw()
        {acquire(r);}

    template <class Y> shared_ptr& operator=(const shared_ptr<Y>& r)
    {
        if (this != &r) {
            release();
            acquire(r);
        }
        return *this;
    }

    X& operator*()  const throw()   {return *itsPtr;}
    X* operator->() const throw()   {return itsPtr;}
    X* get()        const throw()   {return itsPtr;}
    bool unique()   const throw()   {return itsPrev ? itsPrev==this : true;}

#ifdef PRIV
private:
#endif

    X*                          itsPtr;
    mutable const shared_ptr*   itsPrev;
    mutable const shared_ptr*   itsNext;

    void acquire(const shared_ptr& r) throw()

    { // insert this to the list
        itsPtr = r.itsPtr;
        itsNext = r.itsNext;
        itsNext->itsPrev = this;
        itsPrev = &r;
        r.itsNext = this;
    }



    template <class Y> void acquire(const shared_ptr<Y>& r) throw()
    { // insert this to the list
        itsPtr = r.itsPtr;
        itsNext = r.itsNext;
        itsNext->itsPrev = this;
        itsPrev = &r;
        r.itsNext = this;
    }


    void release()
    { // erase this from the list, delete if unique
        if (unique()) delete itsPtr;
        else {
            itsPrev->itsNext = itsNext;
            itsNext->itsPrev = itsPrev;
            itsPrev = itsNext = 0;
        }
        itsPtr = 0;
    }

};

#endif // LINKED_PTR_H

