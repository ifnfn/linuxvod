/*
 * tools.h: Various tools
 *
 * $Id: tools.h 1.93 2006/04/16 10:40:45 kls Exp $
 */

#ifndef __TOOLS_H
#define __TOOLS_H

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>

class CListObject {
private:
	CListObject *prev, *next;
public:
	CListObject(void);
	virtual ~CListObject();
	virtual int Compare(const CListObject &ListObject) const { return 0; }
	void Append(CListObject *Object);
	void Insert(CListObject *Object);
	void Unlink(void);
	int Index(void) const;
	CListObject *Prev(void) const { return prev; }
	CListObject *Next(void) const { return next; }
};

class CListBase {
protected:
	CListObject *objects, *lastObject;
	CListBase(void);
	int count;
public:
	virtual ~CListBase();
	void Add(CListObject *Object, CListObject *After = NULL);
	void Insert(CListObject *Object, CListObject *Before = NULL);
	void Remove(CListObject *Object, bool DeleteObject = true);
	virtual void Move(int From, int To);
	void Move(CListObject *From, CListObject *To);
	virtual void Clear(void);
	CListObject *Get(int Index) const;
	int Count(void) const { return count; }
	void Sort(void);
};

template<class T> class CList : public CListBase {
public:
	T *Get(int Index) const { return (T *)CListBase::Get(Index); }
	T *First(void) const { return (T *)objects; }
	T *Last(void) const { return (T *)lastObject; }
	T *Prev(const T *object) const { return (T *)object->CListObject::Prev(); } // need to call CListObject's members to
	T *Next(const T *object) const { return (T *)object->CListObject::Next(); } // avoid ambiguities in case of a "list of lists"
};

class CHashObject : public CListObject {
	friend class CHashBase;
private:
	unsigned int id;
	CListObject *object;
public:
	CHashObject(CListObject *Object, unsigned int Id) { object = Object; id = Id; }
	CListObject *Object(void) { return object; }
};

class CHashBase {
private:
	CList<CHashObject> **hashTable;
	int size;
	unsigned int hashfn(unsigned int Id) const { return Id % size; }
protected:
	CHashBase(int Size);
public:
	virtual ~CHashBase();
	void Add(CListObject *Object, unsigned int Id);
	void Remove(CListObject *Object, unsigned int Id);
	void Clear(void);
	CListObject *Get(unsigned int Id) const;
	CList<CHashObject> *GetList(unsigned int Id) const;
};

#define HASHSIZE 512

template<class T> class CHash : public CHashBase {
public:
	CHash(int Size = HASHSIZE) : CHashBase(Size) {}
	T *Get(unsigned int Id) const { return (T *)CHashBase::Get(Id); }
};

#endif //__TOOLS_H
