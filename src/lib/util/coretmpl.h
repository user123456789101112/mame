// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    coretmpl.h

    Core templates for basic non-string types.

***************************************************************************/

#pragma once

#ifndef __CORETMPL_H__
#define __CORETMPL_H__

#include "osdcore.h"
#include "corealloc.h"

#include <vector>

// TEMPORARY helper to catch is_pod assertions in the debugger
#if 0
#undef assert
#define assert(x) do { if (!(x)) { fprintf(stderr, "Assert: %s\n", #x); osd_break_into_debugger("Assertion failed"); } } while (0)
#endif


typedef std::vector<UINT8> dynamic_buffer;


// ======================> simple_list

// a simple_list is a singly-linked list whose 'next' pointer is owned
// by the object
template<class _ElementType>
class simple_list
{
	// we don't support deep copying
	simple_list(const simple_list &);
	simple_list &operator=(const simple_list &);

public:
	// construction/destruction
	simple_list()
		: m_head(NULL),
			m_tail(NULL),
			m_count(0) { }

	virtual ~simple_list() { reset(); }

	// simple getters
	_ElementType *first() const { return m_head; }
	_ElementType *last() const { return m_tail; }
	int count() const { return m_count; }

	// remove (free) all objects in the list, leaving an empty list
	void reset()
	{
		while (m_head != NULL)
			remove(*m_head);
	}

	// add the given object to the head of the list
	_ElementType &prepend(_ElementType &object)
	{
		object.m_next = m_head;
		m_head = &object;
		if (m_tail == NULL)
			m_tail = m_head;
		m_count++;
		return object;
	}

	// add the given list to the head of the list
	void prepend_list(simple_list<_ElementType> &list)
	{
		int count = list.count();
		if (count == 0)
			return;
		_ElementType *tail = list.last();
		_ElementType *head = list.detach_all();
		tail->m_next = m_head;
		m_head = head;
		if (m_tail == NULL)
			m_tail = tail;
		m_count += count;
	}

	// add the given object to the tail of the list
	_ElementType &append(_ElementType &object)
	{
		object.m_next = NULL;
		if (m_tail != NULL)
			m_tail = m_tail->m_next = &object;
		else
			m_tail = m_head = &object;
		m_count++;
		return object;
	}

	// add the given list to the tail of the list
	void append_list(simple_list<_ElementType> &list)
	{
		int count = list.count();
		if (count == 0)
			return;
		_ElementType *tail = list.last();
		_ElementType *head = list.detach_all();
		if (m_tail != NULL)
			m_tail->m_next = head;
		else
			m_head = head;
		m_tail = tail;
		m_count += count;
	}

	// insert the given object after a particular object (NULL means prepend)
	_ElementType &insert_after(_ElementType &object, _ElementType *insert_after)
	{
		if (insert_after == NULL)
			return prepend(object);
		object.m_next = insert_after->m_next;
		insert_after->m_next = &object;
		if (m_tail == insert_after)
			m_tail = &object;
		m_count++;
		return object;
	}

	// insert the given object before a particular object (NULL means append)
	_ElementType &insert_before(_ElementType &object, _ElementType *insert_before)
	{
		if (insert_before == NULL)
			return append(object);
		for (_ElementType **curptr = &m_head; *curptr != NULL; curptr = &(*curptr)->m_next)
			if (*curptr == insert_before)
			{
				object.m_next = insert_before;
				*curptr = &object;
				if (m_head == insert_before)
					m_head = &object;
				m_count++;
				return object;
			}
		return object;
	}

	// replace an item in the list at the same location, and remove it
	_ElementType &replace_and_remove(_ElementType &object, _ElementType &toreplace)
	{
		_ElementType *prev = NULL;
		for (_ElementType *cur = m_head; cur != NULL; prev = cur, cur = cur->m_next)
			if (cur == &toreplace)
			{
				if (prev != NULL)
					prev->m_next = &object;
				else
					m_head = &object;
				if (m_tail == &toreplace)
					m_tail = &object;
				object.m_next = toreplace.m_next;
				global_free(&toreplace);
				return object;
			}
		return append(object);
	}

	// detach the head item from the list, but don't free its memory
	_ElementType *detach_head()
	{
		_ElementType *result = m_head;
		if (result != NULL)
		{
			m_head = result->m_next;
			m_count--;
			if (m_head == NULL)
				m_tail = NULL;
		}
		return result;
	}

	// detach the given item from the list, but don't free its memory
	_ElementType &detach(_ElementType &object)
	{
		_ElementType *prev = NULL;
		for (_ElementType *cur = m_head; cur != NULL; prev = cur, cur = cur->m_next)
			if (cur == &object)
			{
				if (prev != NULL)
					prev->m_next = object.m_next;
				else
					m_head = object.m_next;
				if (m_tail == &object)
					m_tail = prev;
				m_count--;
				return object;
			}
		return object;
	}

	// deatch the entire list, returning the head, but don't free memory
	_ElementType *detach_all()
	{
		_ElementType *result = m_head;
		m_head = m_tail = NULL;
		m_count = 0;
		return result;
	}

	// remove the given object and free its memory
	void remove(_ElementType &object)
	{
		global_free(&detach(object));
	}

	// find an object by index in the list
	_ElementType *find(int index) const
	{
		for (_ElementType *cur = m_head; cur != NULL; cur = cur->m_next)
			if (index-- == 0)
				return cur;
		return NULL;
	}

	// return the index of the given object in the list
	int indexof(const _ElementType &object) const
	{
		int index = 0;
		for (_ElementType *cur = m_head; cur != NULL; cur = cur->m_next)
		{
			if (cur == &object)
				return index;
			index++;
		}
		return -1;
	}

private:
	// internal state
	_ElementType *  m_head;         // head of the singly-linked list
	_ElementType *  m_tail;         // tail of the singly-linked list
	int             m_count;        // number of objects in the list
};


// ======================> simple_list_wrapper

// a simple_list_wrapper wraps an existing object with a next pointer so it
// can live in a simple_list without requiring the object to have a next
// pointer
template<class _ObjectType>
class simple_list_wrapper
{
public:
	template<class U> friend class simple_list;

	// construction/destruction
	simple_list_wrapper(_ObjectType *object)
		: m_next(NULL),
			m_object(object) { }

	// operators
	operator _ObjectType *() { return m_object; }
	operator _ObjectType *() const { return m_object; }
	_ObjectType *operator *() { return m_object; }
	_ObjectType *operator *() const { return m_object; }

	// getters
	simple_list_wrapper *next() const { return m_next; }
	_ObjectType *object() const { return m_object; }

private:
	// internal state
	simple_list_wrapper *   m_next;
	_ObjectType *           m_object;
};


// ======================> fixed_allocator

// a fixed_allocator is a simple class that maintains a free pool of objects
template<class _ItemType>
class fixed_allocator
{
	// we don't support deep copying
	fixed_allocator(const fixed_allocator &);
	fixed_allocator &operator=(const fixed_allocator &);

public:
	// construction/destruction
	fixed_allocator() { }

	// allocate a new item, either by recycling an old one, or by allocating a new one
	_ItemType *alloc()
	{
		_ItemType *result = m_freelist.detach_head();
		if (result == NULL)
			result = global_alloc(_ItemType);
		return result;
	}

	// reclaim an item by adding it to the free list
	void reclaim(_ItemType *item) { if (item != NULL) m_freelist.append(*item); }
	void reclaim(_ItemType &item) { m_freelist.append(item); }

	// reclaim all items from a list
	void reclaim_all(simple_list<_ItemType> &_list) { m_freelist.append_list(_list); }

private:
	// internal state
	simple_list<_ItemType>  m_freelist;     // list of free objects
};


#endif
