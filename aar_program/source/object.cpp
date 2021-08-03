#include "object.hpp"

object::object(std::string classname, std::string id) : 
    m_objectClassname(classname),
    m_objectID(id) {}

void object::moveIn(object *obj) {
    m_objectsWithin.insert(obj);
}

void object::moveOut(object *obj) {
    m_objectsWithin.extract(obj);
}

void object::setPosition(float x, float y, float z) {
    m_positionX = x;
    m_positionY = y;
    m_positionZ = z;
}

void object::setAzimuthPitch(float azimuth, float pitch) {
    m_azimuth = azimuth;
    m_pitch = pitch;
}

void object::setLifeState(object::lifeState state) {
    m_lifeState = state;
}

void object::enter(object *obj) {
    m_withinObject = obj;
    obj->moveIn(this);
}

void object::exit() {
    if (m_withinObject) {
        m_withinObject->moveOut(this);
    }
    m_withinObject = nullptr;
}

bool object::insideObject() const {
    return m_withinObject;
}

std::string object::getClassname() const {
    return m_objectClassname;
}

std::string object::getID() const {
    return m_objectID;
}
