
#ifndef SMART_PTR_H
#define SMART_PTR_H

#include <algorithm>

namespace wiz {


    template < class T >
    class SmartPtr
    {
    private:
        class Inner {
        public:
            T* ptr = nullptr;
            int count = 0;
        };
    private:
        Inner* inner = nullptr;
        int option;
    private:
        void quit()
        {
            if (inner) {
                inner->count--;
                if (0 == inner->count) {
                    delete inner;
                }
                inner = nullptr;
            }
        }

        void enter(const SmartPtr<T>& sp)
        {
            //wizard::assertNotnullptr( sp );
            //wizard::assertnullptr( this->ptr );

            if (nullptr == sp.inner)
            {
                this->quit();
            }
            else if (nullptr == this->inner)
            {
                this->inner = sp.inner;
                sp.inner->count++;
                this->option = sp.option; ///
            }
        }

        void initFromOther(const SmartPtr<T>& sp)
        {
            //wizard::assertNotEquals( this, &sp );
           // if( this == sp.getThis() ) { return; } 

            // delete or quit
            if (this->inner)
            {
                if (isOnlyOne()) //
                {
                    remove();
                }
                else
                {
                    quit();
                }
            }

            // enter 
            enter(sp);

            return;
        }
    public:

        SmartPtr(T* ptr = nullptr)
            : option(0)
        {
            // ptr <- new T(~~);
            if (ptr) {
                this->inner = new Inner();
                this->inner->ptr = ptr;
                this->inner->count = 1;
            }
        }

        SmartPtr(T* ptr, const int option) // option 1,2,..
            : option(option)
        {
            // ptr <- new T(~~);
            if (ptr) {
                this->inner = new Inner();
                this->inner->ptr = ptr;
                this->inner->count = 1;
            }
        }
        SmartPtr(const SmartPtr<T>& sp)
            : option(sp.option)
        {
            initFromOther(sp);
        }
        SmartPtr(SmartPtr<T>&& sp)
            :option(sp.option)
        {
            Inner* temp = this->inner;
            this->inner = sp.inner;
            sp.inner = temp;
        }
        virtual ~SmartPtr() /// virtual??
        {
            if (isOnlyOne())
            {
                remove(false);
                quit();
            }
            else
            {
               remove(false);
            }
        }
    public:
        SmartPtr<T>& operator=(T* ptr) {
            if (inner && ptr) {
                inner->ptr = ptr;
            }
            else if (!inner && ptr) {
                inner = new Inner();
                inner->ptr = ptr;
                inner->count = 1;
            }
            return*this;
        }
        SmartPtr<T>& operator=(const SmartPtr<T>& _sp)
        {
            // temp link
            SmartPtr<T> tempLink(_sp);

            initFromOther(tempLink);

            return *this;
        }
        T& operator*()
        {
            // wizard::assertNotnullptr( ptr );
            return (*inner->ptr);
        }
        const T& operator*()const
        {
            // wizard::assertNotnullptr( ptr );
            return (*inner->ptr);
        }
    public:
        bool isOnlyOne()const
        {
            return this->inner && this->inner->count == 1;
        }
        bool isNULL()const
        {
            return nullptr == this->inner || nullptr == this->inner->ptr;
        }
        bool empty()const
        {
            return nullptr == this->inner;
        }
        /// remove return suceess?
        bool remove()
        {
            return remove(true);
        }
        bool remove(const bool bremove) // make private and delete =true, and make public remove() call remove( true ); - 2012.3.5 todo...
        {
            if (empty()) { return false; }
            if (!bremove && isOnlyOne()) { return false; } /// 2013.08.13 false means "no change"??
            if (this->inner->ptr && bremove)
            {
                delete this->inner->ptr; this->inner->ptr = nullptr; // no dynamic.

                quit();
            }
            else
            {
                quit();
            }
            return true;
        }
    public:
        operator T* () {
            if (this->inner) {
                return this->inner->ptr;
            }
            return nullptr;
        }

        const T* operator->()const
        {
            //wizard::assertNotnullptr( ptr );
            return inner->ptr;
        }
        T* operator->()
        {
            //wizard::assertNotnullptr( ptr );
            return inner->ptr;
        }
        T* operator&()
        {
            return inner->ptr;
        }
        const T* operator&()const
        {
            return inner->ptr;
        }
        ///
    public:
        bool hasSameObject(const SmartPtr<T>& wsp) const
        {
            return this->inner == wsp.inner;
        }
    };
}

#endif
