/*
 * Copyright 2010-2012 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef __BX_SPSCQUEUE_H__
#define __BX_SPSCQUEUE_H__

#include <list>

#include "bx.h"
#include "cpu.h"
#include "mutex.h"
#include "uint32_t.h"

namespace bx
{
	// http://drdobbs.com/article/print?articleId=210604448&siteSectionName=
	template <typename Ty>
	class SpScUnboundedQueueLf
	{
		BX_CLASS_NO_COPY_NO_ASSIGNMENT(SpScUnboundedQueueLf);

	public:
		SpScUnboundedQueueLf()
			: m_first(new Node(NULL) )
			, m_divider(m_first)
			, m_last(m_first)
		{
		}

		~SpScUnboundedQueueLf()
		{
			while (NULL != m_first)
			{
				Node* node = m_first;
				m_first = node->m_next;
				delete node;
			}
		}

		void push(Ty* _ptr) // producer only
		{
			m_last->m_next = new Node( (void*)_ptr);
			atomicExchangePtr( (void**)&m_last, m_last->m_next);
			while (m_first != m_divider)
			{
				Node* node = m_first;
				m_first = m_first->m_next;
				delete node;
			}
		}

		Ty* peek() // consumer only
		{
			if (m_divider != m_last)
			{
				Ty* ptr = (Ty*)m_divider->m_next->m_ptr;
				return ptr;
			}

			return NULL;
		}

		Ty* pop() // consumer only
		{
			if (m_divider != m_last)
			{
				Ty* ptr = (Ty*)m_divider->m_next->m_ptr;
				atomicExchangePtr( (void**)&m_divider, m_divider->m_next);
				return ptr;
			}

			return NULL;
		}

	private:
		struct Node
		{
			Node(void* _ptr)
				: m_ptr(_ptr)
				, m_next(NULL)
			{
			}

			void* m_ptr;
			Node* m_next;
		};

		Node* m_first;
		Node* m_divider;
		Node* m_last;
	};

	template<typename Ty>
	class SpScUnboundedQueueMutex
	{
		BX_CLASS_NO_COPY_NO_ASSIGNMENT(SpScUnboundedQueueMutex);

	public:
		SpScUnboundedQueueMutex()
		{
		}

		~SpScUnboundedQueueMutex()
		{
			BX_CHECK(m_queue.empty(), "Queue is not empty!");
		}

		void push(Ty* _item)
		{
			bx::LwMutexScope lock(m_mutex);
			m_queue.push_back(_item);
		}

		Ty* peek()
		{
			bx::LwMutexScope lock(m_mutex);
			if (!m_queue.empty() )
			{
				return m_queue.front();
			}

			return NULL;
		}

		Ty* pop()
		{
			bx::LwMutexScope lock(m_mutex);
			if (!m_queue.empty() )
			{
				Ty* item = m_queue.front();
				m_queue.pop_front();
				return item;
			}

			return NULL;
		}

	private:
		bx::LwMutex m_mutex;
		std::list<Ty*> m_queue;
	};

#if BX_CONFIG_SPSCQUEUE_USE_MUTEX
#	define SpScUnboundedQueue SpScUnboundedQueueMutex
#else
#	define SpScUnboundedQueue SpScUnboundedQueueLf
#endif // BX_CONFIG_SPSCQUEUE_USE_MUTEX

	template <typename Ty>
	class SpScBlockingUnboundedQueue
	{
		BX_CLASS_NO_COPY_NO_ASSIGNMENT(SpScBlockingUnboundedQueue);

	public:
		SpScBlockingUnboundedQueue()
		{
		}

		~SpScBlockingUnboundedQueue()
		{
		}

		void push(Ty* _ptr) // producer only
		{
			m_queue.push( (void*)_ptr);
			m_count.post();
		}

		Ty* peek() // consumer only
		{
			return (Ty*)m_queue.peek();
		}

		Ty* pop(int32_t _msecs = -1) // consumer only
		{
			if (m_count.wait(_msecs) )
			{
				return (Ty*)m_queue.pop();
			}

			return NULL;
		}

	private:
		Semaphore m_count;
		SpScUnboundedQueue<void> m_queue;
	};

} // namespace bx

#endif // __BX_RINGBUFFER_H__
