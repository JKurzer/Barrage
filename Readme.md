
# install
### You MUST install CMake to use this plugin.
A CMAKE installer is provided for the sake of your sanity and mine. Any modern CMake should be fine, but I don't recommend anything below 3.27.

### IL Mismatch error 
You may hit an issue where an IL mismatch occurs. If this happens, let me know, and we'll compare environments. this arises when cmake and UE's build system use different toolchain components or when CMake uses one linker and another compiler.

# Tombstones
Because we can't control certain aspects of timing and because we may need to roll back, we use tombstoning. Tombstoning is the practice of marking a record as dead without deleting it. It comes in three varieties, ephemeral, permanent, and timed. We use a timed marking, so you have a certain number of cycles before an object is nulled or returned to the pool once it is marked as tombstoned. Any new usage of that object is contraindicated, but this is intended to help ensure that in an artillery gun, we need a little less boiler plate and in certain cases, that we can reliably make statements about lifecycles in the general case at all.

Right now, it is used primarily on the FBPrimitive which lives underneath the critical FBLet class. Here, any non-zero value for the tombstone is the same, effectively, as a nullity for the purposes of any new operation. Again, instead of just reference counting and deleting - tombstones are ref counted but have cases where there may be single-source authoritative answers to the question of the backing objects' lifecycles. For cases like this, that arise in our case because there MUST be an authoritative single source answer to the alive/dead question for a rigid body, we still want all the advantages of ref counting and we want to be able to revert that decision for faster rollbacks or for pooling purposes. Tombstoning is a simple and common approach.