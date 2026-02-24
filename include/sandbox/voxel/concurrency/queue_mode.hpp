#pragma once

namespace sandbox::voxel::concurrency {

enum class QueueMode {
    mutex_cv,
    lock_free_mpsc,
};

} // namespace sandbox::voxel::concurrency
