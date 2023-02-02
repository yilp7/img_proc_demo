#include <cstdlib>

namespace Euresys {
namespace MultiCam {
namespace Internal {

template < class CItem, int localsize = 512 >
class AssociativeArray : public Container < CItem >
{
        AssociativeArray<CItem, localsize> *next;
        CItem items[localsize];
        int keys[localsize];
        int used;
    public:
        /** Construct an associative array.
        **/
        AssociativeArray() : next(0), used(0) {
            int i;
            for(i = 0; i < localsize; i++) {
                items[i] = NULL;
                keys[i] = -1;
            }
        }

        ~AssociativeArray() {
            if (next)
                delete next;
        }

        void Add(int key, CItem item) {
            int i;
            i = Find(key);
            if (isValidIndex(i) || (!isValidIndex(i) && used < localsize)) {
                if (!isValidIndex(i)) {
                    i = used;
                    used++;
                }
                items[i] = item;
                keys[i] = key;
            } else {
                if (!next) {
                    next = new AssociativeArray<CItem, localsize>();
                }
                next->Add(key, item);
            }
        }

        // for compatibility with the container interface
        virtual int Add(CItem &) {
            // NOT IMPLEMENTED !!
            return 0;
        }

        // for compatibility with the container interface
        virtual void Assign(int idx, CItem item) {
            Add(idx, item);
        }

        virtual int GetCount() const {
            int size;

            size = used;
            if (next) size  += next->GetCount();
            return size;
        }

        int Find(int key) const {
            int i;
            for (i = 0; i < used && i < localsize; i++) {
                if (keys[i] == key)
                    return i;
            }
            return -1;
        }

        bool isValidIndex(int i) const {
            return i >= 0 && i < used && i < localsize;
        }

        void Remove(int key) {
            int i = Find(key);
            if (isValidIndex(i)) {
                items[i] = NULL;            
            }
        }

        CItem Get(int key) const {
            int i;
            i = Find(key);
            if (isValidIndex(i))
                return items[i];
            else if (next) 
                return next->Get(key);
            
            return NULL;
        }

        // for compatibility with the container interface
        CItem At(int key) const {
            return Get(key);
        }

        void DeleteAll() {
            int i;
            for (i=0; i < used; i++) {
                delete items[i];
                items[i] = NULL;
            }
            if (next) {
                next->DeleteAll();
                delete next;
                next = NULL;
            }
        }
};

      
      
      // ********************************************************************************************
      // List: simple list class
      // -----------------------
      
      template <class T, int N> class List : public Container<T>
      {
        protected:
            int count, allocCount;
            T *list;
            
        public:
            List();
            virtual ~List();
            
            T operator[](int Index) const { return At(Index); }
            T At(int Index) const { return list[Index]; }
            int GetCount() const { return count; }
            int Add(T &item);
      };
      
      template <class T, int N>
          List<T, N>::List()
      {
          count = 0;
          allocCount = 0;
          list = NULL;
      }
      
      template <class T, int N>
          List<T, N>::~List()
      {
          if (list != NULL)
              free(list);
      }
      
      template <class T, int N>
          int List<T, N>::Add(T &item)
      {
          int element = count++;
          
          if (count >= allocCount)
          {
              allocCount += N;
              list = (T *)realloc(list, sizeof(T) * allocCount);
          }
          
          list[element] = item;
          
          return element;
      }
    
    
}
}
}
