#ifndef ECSsystem_H 
#define ECSsystem_H

#include <cstdint>
#include <vector>
#include <queue>
#include <bitset>
#include <array>
#include <cassert>




namespace ECSystem {

	using Entity = std::uint32_t;
	static const Entity MAX_ENTITIES = 5000; // make explicit internal linkage 
	using ComponentType = std::uint8_t;
	static const ComponentType MAX_COMPONENTS = 32;
	using Signature = std::bitset<MAX_COMPONENTS>;

	class EntityManager {
	public:
		EntityManager() {
			for (Entity e = 0; e < MAX_ENTITIES; ++e) {
				mAvailableEntities.push(e);
			}
		}

		Entity CreateEntity() {
			assert(mLivingEntityCount < MAX_ENTITIES && "Too many entities in existence.");

			// Take an ID from the front of the queue
			Entity id = mAvailableEntities.front();
			mAvailableEntities.pop();
			++mLivingEntityCount;

			return id;
		}

		void DestroyEntity(Entity entity) {
			assert(entity < MAX_ENTITIES && "Entity out of range.");
			mSignatures[entity].reset();
			// Put the destroyed ID at the back of the queue
			mAvailableEntities.push(entity);
			--mLivingEntityCount;
		}

		void SetSignature(Entity entity, Signature signature) {
			assert(entity < MAX_ENTITIES && "Entity out of range.");
			mSignatures[entity] = signature;
		}

		Signature GetSignature(Entity entity)
		{
			assert(entity < MAX_ENTITIES && "Entity out of range.");

			// Get this entity's signature from the array
			return mSignatures[entity];
		}

	private:
		// Queue of unused entity IDs
		std::queue<Entity> mAvailableEntities{};
		// Array of signatures where the index corresponds to the entity ID
		std::array<Signature, MAX_ENTITIES> mSignatures{};
		// Total living entities - used to keep limits on how many exist
		uint32_t mLivingEntityCount{};
	};

}


#endif // !ECSsystem_H
