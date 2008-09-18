/*
 * tools.c: Various tools
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: tools.c 1.120 2006/08/12 13:30:07 kls Exp $
 */

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
extern "C" {
#ifdef boolean
#define HAVE_BOOLEAN
#endif
#undef boolean
}
#include <stdarg.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/vfs.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

#include "list.h"

CListObject::CListObject(void)
{
	prev = next = NULL;
}

CListObject::~CListObject()
{
}

void CListObject::Append(CListObject *Object)
{
	next = Object;
	Object->prev = this;
}

void CListObject::Insert(CListObject *Object)
{
	prev = Object;
	Object->next = this;
}

void CListObject::Unlink(void)
{
	if (next)
		next->prev = prev;
	if (prev)
		prev->next = next;
	next = prev = NULL;
}

int CListObject::Index(void) const
{
	CListObject *p = prev;
	int i = 0;

	while (p) {
		i++;
		p = p->prev;
	}
	return i;
}

// --- CListBase -------------------------------------------------------------

CListBase::CListBase(void)
{
	objects = lastObject = NULL;
	count = 0;
}

CListBase::~CListBase()
{
	Clear();
}

void CListBase::Add(CListObject *Object, CListObject *After)
{
	if (After && After != lastObject) {
		After->Next()->Insert(Object);
		After->Append(Object);
	}
	else {
		if (lastObject)
			lastObject->Append(Object);
		else
			objects = Object;
		lastObject = Object;
	}
	count++;
}

void CListBase::Insert(CListObject *Object, CListObject *Before)
{
	if (Before && Before != objects) {
		Before->Prev()->Append(Object);
		Before->Insert(Object);
	}
	else {
		if (objects)
			objects->Insert(Object);
		else
			lastObject = Object;
		objects = Object;
	}
	count++;
}

void CListBase::Remove(CListObject *Object, bool DeleteObject)
{
	if (Object == objects)
		objects = Object->Next();
	if (Object == lastObject)
		lastObject = Object->Prev();
	Object->Unlink();
	if (DeleteObject)
		delete Object;
	count--;
}

void CListBase::Move(int From, int To)
{
	Move(Get(From), Get(To));
}

void CListBase::Move(CListObject *From, CListObject *To)
{
	if (From && To) {
		if (From->Index() < To->Index())
			To = To->Next();
		if (From == objects)
			objects = From->Next();
		if (From == lastObject)
			lastObject = From->Prev();
		From->Unlink();
		if (To) {
			if (To->Prev())
				To->Prev()->Append(From);
			From->Append(To);
		}
		else {
			lastObject->Append(From);
			lastObject = From;
		}
		if (!From->Prev())
			objects = From;
	}
}

void CListBase::Clear(void)
{
	while (objects) {
		CListObject *object = objects->Next();
		delete objects;
		objects = object;
	}
	objects = lastObject = NULL;
	count = 0;
}

CListObject *CListBase::Get(int Index) const
{
	if (Index < 0)
		return NULL;
	CListObject *object = objects;
	while (object && Index-- > 0)
		object = object->Next();
	return object;
}

static int CompareListObjects(const void *a, const void *b)
{
	const CListObject *la = *(const CListObject **)a;
	const CListObject *lb = *(const CListObject **)b;
	return la->Compare(*lb);
}

void CListBase::Sort(void)
{
	int n = Count();
	CListObject *a[n];
	CListObject *object = objects;
	int i = 0;
	while (object && i < n) {
		a[i++] = object;
		object = object->Next();
	}
	qsort(a, n, sizeof(CListObject *), CompareListObjects);
	objects = lastObject = NULL;
	for (i = 0; i < n; i++) {
		a[i]->Unlink();
		count--;
		Add(a[i]);
	}
}

// --- CHashBase -------------------------------------------------------------

CHashBase::CHashBase(int Size)
{
	size = Size;
	hashTable = (CList<CHashObject>**)calloc(size, sizeof(CList<CHashObject>*));
}

CHashBase::~CHashBase(void)
{
	Clear();
	free(hashTable);
}

void CHashBase::Add(CListObject *Object, unsigned int Id)
{
	unsigned int hash = hashfn(Id);
	if (!hashTable[hash])
		hashTable[hash] = new CList<CHashObject>;
	hashTable[hash]->Add(new CHashObject(Object, Id));
}

void CHashBase::Remove(CListObject *Object, unsigned int Id)
{
	CList<CHashObject> *list = hashTable[hashfn(Id)];
	if (list) {
		for (CHashObject *hob = list->First(); hob; hob = list->Next(hob)) {
			if (hob->object == Object) {
				list->Remove(hob);
				break;
			}
		}
	}
}

void CHashBase::Clear(void)
{
	for (int i = 0; i < size; i++) {
		delete hashTable[i];
		hashTable[i] = NULL;
	}
}

CListObject *CHashBase::Get(unsigned int Id) const
{
	CList<CHashObject> *list = hashTable[hashfn(Id)];
	if (list) {
		for (CHashObject *hob = list->First(); hob; hob = list->Next(hob)) {
			if (hob->id == Id)
				return hob->object;
		}
	}
	return NULL;
}

CList<CHashObject> *CHashBase::GetList(unsigned int Id) const
{
	return hashTable[hashfn(Id)];
}
