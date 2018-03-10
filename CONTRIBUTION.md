If you found a bug - feel free to create an issue for it, or even propose a patch to "contrib" branch.

If you want to contribute a patch - you're assumed to agree with project license, minor bug fixes are not supposed to change copyrights.

If you propose a patch with a bug fix - please add appropriate test for it (near other tests).

Due to project specifics (embedded) - it's likely to be very hard to reproduce some hardware/compiler dependent issues,
also I don't have CI builders for testing regular platforms, so I may be reluctant to apply such updates.

If you want to create a fork and rework some stuff to optimize for your target platform (or by other reasons) - please update ECL_VERSION_BRANCH macro to be anything but "master",
preferably to clearly identify your fork.
