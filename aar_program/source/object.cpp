#include "object.hpp"

object::object(std::string classname, std::string id, std::string name) : 
    m_objectClassname(classname),
    m_objectID(id),
    m_name(name)
{}

void object::moveIn(std::string objectUID) {
    m_objectsWithin.insert(objectUID);
}

void object::moveOut(std::string objectUID) {
    m_objectsWithin.extract(objectUID);
}

void object::setLifeState(object::lifeState state) {
    m_lifeState = state;
}

void object::enter(std::string object) {
    m_withinObject = object;
}

void object::exit() {
    m_withinObject = "";
}

bool object::insideObject() const {
    return m_withinObject != "";
}

std::string object::getClassname() const {
    return m_objectClassname;
}

std::string object::getID() const {
    return m_objectID;
}

std::string object::getName() const {
    return m_name;
}

std::string object::getVehicleIn() const {
	return m_withinObject;
}

const std::set<std::string> &object::getCargo() const{
    return m_objectsWithin;
}
