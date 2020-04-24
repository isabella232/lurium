#pragma once
/*
Quadtree diffing algo "v2" - framework for detecting state changes between 
two frames in a game server for a client.

Public classes
	game_engine<> - a quadtree with diffing capabilities, your game uses this
	game_motion_object<> - your queryable game objects inherit from this

Internal classes
	game_object<> - this is the base class for all objects in the collider
	game_motion_move<> - a special kind of game_object<> for movements

Template arguments
	KEYTYPE - the numeric type of quadtree rectangles, typically int
	or float

	typeT - the enum type for your game object identifiers. The enum 
	must have an entry for "motion"

	object_type_move - the enum value in your game object identifier enum 
	given by typeT which represents "motion"

Flow
	The game server typically runs a timer to update game state and transmit 
	state changes to the players:
		1. Update game state -> tick()
		2. Transmit updated state to all players
		3. Commit game state changes -> tick_done()

	Spawning new objects - game_engine::spawn()
	When a player requests to join, the server spawns the player into the game 
	some time before step 1. Part of updating the game state might involve 
	spawning new objects. 

	Moving objects - game_engine::update_motion()
	If an object can move, its bounds should be updated exactly once per frame,
	and never while spawning or despawning. Static objects with fixed position
	and size should not call update_motion()

	Despawning new objects - game_engine::despawn()
	Called when a player disconnects, or when the player dies. Or f.ex when 
	some resource was consumed as part of updating the game state.

	Commit game state changes - game_engine::process_spawns()
	As part of finalizing the tick, after all diffing queries are done, your 
	game must call process_spawns() to prepare for next frame. 

	Querying objects - game_engine::collider::query()

	Querying object diffs - game_engine::query_diff()
	Returns the difference between previous and current frame, given the 
	previous and current view rectangles. The diff is a list of objects 
	annotated with a diff_type indicating which applies of insert, update or 
	delete. Spwaning objects, or objects coming into view are inserted. Moving 
	objects are updated. Despawning or objects going out of view are deleted.
	Thus, providing a mechanism to transmit only game state changes between 
	the server and client.

Game objects memory management
	The quadtree is doing some memory management of its own. It assumes to work
	on classes deriving from game_motion_object<> and stored in preallocated 
	std::vectors. All elements must have active=false initially. Objects are 
	then allocated by setting active=true. This provides for stable pointers, 
	which are then stored in the quadtree and can be queried for.
*/

#include <set>

template <typename typeT>
struct game_object {
	typeT type;
	int object_id;
	bool active; // could rename to "allocated"
	bool is_spawning;
	bool is_despawning;

	game_object() {
		active = false;
		is_spawning = false;
		is_despawning = false;
	}
};

template <typename KEYTYPE, typename typeT>
struct game_motion_object;

template <typename KEYTYPE, typename typeT>
struct game_motion_spawn {
	rect<KEYTYPE> rc;
	game_motion_object<KEYTYPE, typeT>* ref;
};

template <typename KEYTYPE, typename typeT>
struct game_motion_despawn {
	rect<KEYTYPE> rc;
	game_motion_object<KEYTYPE, typeT>* ref;
};

template <typename KEYTYPE, typename typeT>
struct game_motion_move : game_object<typeT> {
	rect<KEYTYPE> rc;
	game_motion_object<KEYTYPE, typeT>* ref;
};

template <typename KEYTYPE, typename typeT>
struct game_motion_object : game_object<typeT> {
	rect<KEYTYPE> bounds;
	std::vector<game_motion_move<KEYTYPE, typeT> > motion;

	game_motion_object() {
		motion.reserve(1);
	}
};

template <typename KEYTYPE, typename typeT, typeT object_type_move>
struct game_engine {

	std::vector<game_motion_spawn<KEYTYPE, typeT> > spawns;
	std::vector<game_motion_despawn<KEYTYPE, typeT> > despawns;

	typedef quadtree<KEYTYPE, game_object<typeT>*> quadtree_type;
	quadtree_type collider;

	game_engine() {
		spawns.reserve(200);
		despawns.reserve(200);
	}

	void process_spawns() {

		for (auto& spawn : spawns) {
			assert(spawn.ref->is_spawning && spawn.ref->active);
			spawn.ref->is_spawning = false;
		}
		spawns.clear();

		for (auto& despawn : despawns) {
			assert(despawn.ref->is_despawning && despawn.ref->active);
			collider.remove(despawn.rc, despawn.ref);
			despawn.ref->active = false;
			despawn.ref->is_despawning = false;
		}
		despawns.clear();
	}

	void spawn(game_motion_object<KEYTYPE, typeT>& object) {
		printf("spaning obj %i\n", object.object_id);

		object.active = true;
		object.is_spawning = true;
		collider.insert(object.bounds, &object);

		assert(spawns.size() <= spawns.capacity());
		spawns.push_back(game_motion_spawn<KEYTYPE, typeT>());
		auto& spawn = spawns.back();
		spawn.rc = object.bounds;
		spawn.ref = &object;
	}

	void despawn(game_motion_object<KEYTYPE, typeT>& object) {
		assert(!object.is_despawning);

		object.is_despawning = true;

		for (auto& motion : object.motion) {
			collider.remove(motion.rc, &motion);
		}
		object.motion.clear();

		assert(despawns.size() <= despawns.capacity());
		despawns.push_back(game_motion_despawn<KEYTYPE, typeT>());
		auto& despawn = despawns.back();
		despawn.rc = object.bounds;
		despawn.ref = &object;
	}


	void update_motion(game_motion_object<KEYTYPE, typeT>& object, const rect<KEYTYPE>& new_bounds) {
		// either foreach the rects, then remove exactly, or change the quadtree to return valuetype again
		assert(!object.is_despawning);
		assert(!object.is_spawning);

		collider.move(object.bounds, new_bounds, &object);

		if (object.motion.empty()) {
			object.motion.resize(1);

			auto& motion = object.motion[0];
			motion.type = object_type_move;
			motion.ref = &object;
			motion.rc = object.bounds;
			collider.insert(motion.rc, &motion);
		}
		else {
			auto& motion = object.motion[0];
			collider.move(motion.rc, object.bounds, &motion);
			motion.rc = object.bounds;
		}
		object.bounds = new_bounds;

	}


	typedef typename quadtree_type::object_type T;

	// TODO: callback for diff; build serializable directly
	void query_diff(rect<KEYTYPE> prev_rect, rect<KEYTYPE> next_rect, std::vector<diff_object<T>>& result) {
		rect<KEYTYPE> combine_rect = next_rect;
		if (prev_rect.left != prev_rect.right && prev_rect.top != prev_rect.bottom) {
			combine_rect.extend_bound(prev_rect.left, prev_rect.top);
			combine_rect.extend_bound(prev_rect.right, prev_rect.bottom);
		}

		std::vector<typename quadtree_type::value_type> combine_objects;
		collider.query(combine_rect, combine_objects);

		// TODO: diff-callback så kan vi bygge to arrays, og fjerne en loop
		std::vector<game_motion_move<KEYTYPE, typeT>*> prev_objects;
		std::set<game_motion_object<KEYTYPE, typeT>*> all_objects; // NOTE: std::set vs std::vector+std::sort+std::unique

		for (auto& o : combine_objects) {
			if (o.value->type == object_type_move) {
				auto mo = (game_motion_move<KEYTYPE, typeT>*)o.value;
				assert(mo->ref->active);

				if (mo->rc.intersects(prev_rect)) {
					prev_objects.push_back(mo);
					all_objects.insert(mo->ref);
				}
			}
			else {
				all_objects.insert((game_motion_object<KEYTYPE, typeT>*)o.value);
			}
		}

		for (auto object : all_objects) {
			auto prev_object = std::find_if(prev_objects.begin(), prev_objects.end(), [object](const game_motion_move<KEYTYPE, typeT>* prev_object) {
				// NOTE: comparing by id instead of by reference, because object may have been reallocated
				// NOTE/TODO: above/current was added during frantic debug; dont think its true - reconsider?
				return prev_object->ref->object_id == object->object_id;
			});

			auto is_spawning = object->is_spawning;
			auto is_despawning = object->is_despawning;
			auto is_stationary = object->motion.empty();
			auto in_prev_rect = !is_spawning && (prev_object != prev_objects.end() || (is_stationary && object->bounds.intersects(prev_rect)));
			auto in_next_rect = !is_despawning && object->bounds.intersects(next_rect);

			if (in_next_rect && !in_prev_rect) {
				result.push_back(diff_object<T>(diff_modify_type_add, object));
			}
			else if (in_next_rect && in_prev_rect) {
				if (prev_object != prev_objects.end()) {
					result.push_back(diff_object<T>(diff_modify_type_update, object));
				}
			}
			else if (!in_next_rect && in_prev_rect) {
				result.push_back(diff_object<T>(diff_modify_type_remove, object));
			}
			else if (!in_next_rect && !in_prev_rect) {
				; // in combine_rect, but not in prev_ nor next_rect -> out of band
			}

		}
	}

};
