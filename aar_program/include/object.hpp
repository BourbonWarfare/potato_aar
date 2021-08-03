// object.hpp
// An object representation. All objects within ARMA are assumed to have the same properties
#pragma once
#include <set>
#include <string>

class object {
	public:
		enum class lifeState {
			ALIVE,
			UNCONCIOUS,
			DEAD
		};

	private:
		// Any objects that are "inside" this. Can be for storage, crew, etc
		std::set<object*> m_objectsWithin;
		// The object we are currently inside
		object *m_withinObject = nullptr;

		lifeState m_lifeState = lifeState::ALIVE;

		// all position ASL
		float m_positionX = 0.f;
		float m_positionY = 0.f;
		float m_positionZ = 0.f;

		// degrees
		float m_azimuth = 0.f;
		float m_pitch = 0.f;

		std::string m_objectClassname = "";
		std::string m_objectID = "";

	public:
		object(std::string classname, std::string id);

		void moveIn(object *obj);
		void moveOut(object *obj);

		void setPosition(float x, float y, float z);
		void setAzimuthPitch(float azimuth, float pitch);

		void setLifeState(lifeState state);

		void enter(object *obj);
		void exit();
		bool insideObject() const;

		std::string getClassname() const;
		std::string getID() const;

};


