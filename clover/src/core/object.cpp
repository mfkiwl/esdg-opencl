/*
 * Copyright (c) 2011, Denis Steckelmacher <steckdenis@yahoo.fr>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file object.cpp
 * \brief Reference-counted object tree
 */

#include "object.h"
#include <iostream>

using namespace Coal;

pthread_mutex_t Object::KnownObjectsMutex = PTHREAD_MUTEX_INITIALIZER;

static std::list<Object *>& getKnownObjects()
{
    static std::list<Object *> known_objects;
    return known_objects;
}


Object::Object(Type type, Object *parent)
: p_references(1), p_parent(parent), p_type(type), p_release_parent(true)
{
    if (parent)
        parent->reference();

    // Add object in the list of known objects
    pthread_mutex_lock(&Object::KnownObjectsMutex);
    getKnownObjects().push_front(this);
    p_it = getKnownObjects().begin();
    pthread_mutex_unlock(&Object::KnownObjectsMutex);
}

Object::~Object()
{
#ifdef DBG_OBJ
  std::cerr << "Destructing Object (" << std::hex << (unsigned long)this
    << ")" << std::endl;
#endif

    if (p_parent && p_parent->dereference() && p_release_parent)
        delete p_parent;

    pthread_mutex_lock(&Object::KnownObjectsMutex);
    // Remove object from the list of known objects
    getKnownObjects().erase(p_it);
    pthread_mutex_unlock(&Object::KnownObjectsMutex);

#ifdef DBG_OBJ
  std::cerr << "Successfully destroyed Object (" << std::hex
    << (unsigned long)this << ")" << std::endl;
#endif
}

void Object::reference()
{
    p_references++;
}

bool Object::dereference()
{
    p_references--;
    return (p_references == 0);
}

void Object::setReleaseParent (bool release)
{
    p_release_parent = release;
}

unsigned int Object::references() const
{
    return p_references;
}

Object *Object::parent() const
{
    return p_parent;
}

Object::Type Object::type() const
{
    return p_type;
}

bool Object::isA(Object::Type type) const
{
#ifdef DBG_OBJ
  std::cerr << "Object at " << std::hex << (unsigned long)this
    << ", isA: ";
  switch(type) {
  case T_Device:
    std::cerr << "Device";
    break;
  case T_CommandQueue:
    std::cerr << "CommandQueue";
    break;
  case T_Event:
    std::cerr << "Event";
    break;
  case T_Context:
    std::cerr << "Context";
    break;
  case T_Kernel:
    std::cerr << "Kernel";
    break;
  case T_MemObject:
    std::cerr << "MemObject";
    break;
  case T_Program:
    std::cerr << "Program";
    break;
  case T_Sampler:
    std::cerr << "Sampler";
    break;
  }
  std::cerr << std::endl;
#endif
    // Check for null values
    if (this == 0) {
#ifdef DBG_OBJ
      std::cerr << "this is null, returning false" << std::endl;
#endif
      return false;
    }

    pthread_mutex_lock(&Object::KnownObjectsMutex);
    // Check that the value isn't garbage or freed pointer
    std::list<Object *>::const_iterator it = getKnownObjects().begin(),
                                        e = getKnownObjects().end();
    while (it != e)
    {
        if (*it == this) {
          pthread_mutex_unlock(&Object::KnownObjectsMutex);
#ifdef DBG_OBJ
            std::cerr << "found object in known objects" << std::endl;
#endif
          return this->type() == type;
        }

        ++it;
    }
    pthread_mutex_unlock(&Object::KnownObjectsMutex);

#ifdef DBG_OBJ
    std::cerr << "returning false" << std::endl;
#endif
    return false;
}
