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
		std::set<std::string> m_objectsWithin;
		// The object we are currently inside
		std::string m_withinObject = "";

		lifeState m_lifeState = lifeState::ALIVE;

        const std::string m_objectClassname = "";
        const std::string m_objectID = "";
        const std::string m_name = "";

	public:
        // all position ASL
        float positionX = 0.f;
        float positionY = 0.f;
        float positionZ = 0.f;

        // degrees
        float azimuth = 0.f;
        float pitch = 0.f;

		object(std::string classname, std::string id, std::string name);

		void moveIn(std::string objectUID);
		void moveOut(std::string objectUID);

		void setLifeState(lifeState state);

		void enter(std::string object);
		void exit();
		bool insideObject() const;

		std::string getClassname() const;
		std::string getID() const;
        std::string getName() const;

        std::string getVehicleIn() const;
        const std::set<std::string> &getCargo() const;

};


