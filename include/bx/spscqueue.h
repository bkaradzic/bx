/*
 * Copyright 2010-2017 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

#ifndef BX_SPSCQUEUE_H_HEADER_GUARD
#define BX_SPSCQUEUE_H_HEADER_GUARD

#include "bx.h"
#include "cpu.h"
#include "mutex.h"
#include "semaphore.h"
#include "uint32_t.h"

namespace bx
{
	// http://drdobbs.com/article/print?articleId=210604448&siteSectionName=
	template <typename Ty>
	class SpScUnboundedQueue
	{
		BX_CLASS(SpScUnboundedQueue
			, NO_COPY
			, NO_ASSIGNMENT
			);

	public:
		SpScUnboundedQueue()
			: m_first(new Node(NULL) )
			, m_divider(m_first)
			, m_last(m_first)
		{
		}

		~SpScUnboundedQueue()
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

#if BX_CONFIG_SUPPORTS_THREADING
	template <typename Ty>
	class SpScBlockingUnboundedQueue
	{
		BX_CLASS(SpScBlockingUnboundedQueue
			, NO_COPY
			, NO_ASSIGNMENT
			);

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
#endif // BX_CONFIG_SUPPORTS_THREADING

} // namespace bx

#endif // BX_SPSCQUEUE_H_HEADER_GUARD
