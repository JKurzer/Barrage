## Credits
This work would not have been possible without Andrea and the community at Momento Games.

# Install
### 1: UE4CMake
You must install the UE4CMake plugin, a slightly modified fork of which can be found in a repository adjacent to this one.  
It's not optional. We may manage to defactor it eventually, but there are no plans to do so at this moment.
### 2: You MUST install CMake to use this plugin.
A CMAKE installer is provided for the sake of your sanity and mine. Any modern CMake should be fine, but I don't recommend anything below 3.27. Again, it also requires UE4CMake, of which I host a [slightly modified repo](https://github.com/JKurzer/UE4CMake) that is known-working with this build. Please note, the compiler version is pinned intentionally, so you may need to install a specific tool chain.

### 3: Compiler Version Pinning
In Source/JoltPhysics/JoltPhysics.Build.cs you will find that this plugin not only relies on UE4CMake, but uses a specific  
version of the VC toolchain. This version or a suitable comparable should be installed if you've installed UE5.4. If you have not, you can find a copy of the VC compiler 14.38.33130 in the zip of the same name at root. You will need to need to run the setup file for the compiler as well. However, this is provided without warranty or promise of support. If possible, please install the correct version using the VS installer or as part of your UE install and associated workload.

## Known Install issues
### IL Mismatch error 
You may hit an issue where an IL mismatch occurs. I have had no luck reproducing this locally, and it appears to be due to a specific installation order issue. If this happens, let me know, and we'll compare environments. this arises when cmake and UE's build system use different toolchain components or when CMake uses one linker and another compiler.

# Oddities Of Note
### Tombstones
Because we can't control certain aspects of timing and because we may need to roll back, we use tombstoning. Tombstoning is the practice of marking a record as dead without deleting it. It comes in three varieties, ephemeral, permanent, and timed. We use a timed marking, so you have a certain number of cycles before an object is nulled or returned to the pool once it is marked as tombstoned. Any new usage of that object is contraindicated, but this is intended to help ensure that in an artillery gun, we need a little less boiler plate and in certain cases, that we can reliably make statements about lifecycles in the general case at all.

Right now, it is used primarily on the FBPrimitive which lives underneath the critical FBLet class. Here, any non-zero value for the tombstone is the same, effectively, as a nullity for the purposes of any new operation. Again, instead of just reference counting and deleting - tombstones are ref counted but have cases where there may be single-source authoritative answers to the question of the backing objects' lifecycles. For cases like this, that arise in our case because there MUST be an authoritative single source answer to the alive/dead question for a rigid body, we still want all the advantages of ref counting and we want to be able to revert that decision for faster rollbacks or for pooling purposes. Tombstoning is a simple and common approach.

### Barrage Client Thread Count
The Barrage Client Thread Count is implicitly capped at 64 by ```ThreadAcc[64]``` in BarrageDispatch. Client threads must be ordered to offer determinism with full certainty, and we would like to be able to select the correct queue for a thread to use without locking and in o(1) time. This leaves a very very very small number of data structures for us to use. I eventually settled on a mix of thread local storage and a fixed size add-only accumulator. Unlike a circular buffer, add only accumulators can only be added to. Nothing overwrites. These fixed size versions can only accept a fixed number of adds, but in exchange, always occupy a fixed place in memory. This means that writes to them only block other writes, not reads. It's sort of a bizarre and delightful thing. Except that it uses thread_local so you uh, might wanna take a look at that if you're going to use this.

### Skeleton Key Plugin 
Unfortunately, there's not a really good idiomatic way around the fact that we need a way to disambiguate keys that can be pulled in by systems that actually have no need for vis or dependency on keys that might be disambig'd rel to each other. Rather than doing something baroque to enable sophisticated type inference or erasure, we use a key suffix. The machinery for this lives in the [SkeletonKey](https://github.com/JKurzer/SkeletonKey) plugin. 
