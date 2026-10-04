/* Private, render-thread-only bridge for the separately built Metal candidate.
 * Not a GLES entry point: ordinary glGetQueryObject* keeps boolean semantics.
 * Returns failure for invalid/active/non-occlusion queries. No game code here. */
#include "libANGLE/Context.h"
#include "libANGLE/Query.h"
#include "libANGLE/renderer/metal/ContextMtl.h"
#include "libANGLE/renderer/metal/QueryMtl.h"
#include "libGLESv2/global_state.h"
#include <cstring>
#include <limits>

extern "C" int halopad_angle_buffer_read_write_enabled(void)
{
    auto *context = gl::GetValidGlobalContext();
    return context ? rx::mtl::GetImpl(context)->getDisplay()->getFeatures().allowBufferReadWrite.enabled : -1;
}

extern "C" int halopad_angle_query_samples(unsigned id, unsigned *samples)
{
    if (!samples) return 0;
    *samples = 0;
    gl::Context *context = gl::GetValidGlobalContext();
    if (!context) return 0;
    gl::Query *query = context->getQuery(gl::QueryID{id});
    if (!query || context->getState().isQueryActive(query) ||
        (query->getType() != gl::QueryType::AnySamples &&
         query->getType() != gl::QueryType::AnySamplesConservative)) return 0;
    // Reuse the normal finish/resolve/wait path, then read the same resolved
    // buffer. Do not return the boolean conversion as if it were a count.
    GLuint boolean;
    if (query->getResult(context, &boolean) != angle::Result::Continue) return 0;
    auto *metalQuery = static_cast<rx::QueryMtl *>(query->getImplementation());
    const auto &buffer = metalQuery->getVisibilityResultBuffer();
    if (!buffer) return 0;
    auto bytes = buffer->mapReadOnly(rx::mtl::GetImpl(context));
    uint64_t count;
    std::memcpy(&count, bytes.data(), sizeof(count));
    buffer->unmap(rx::mtl::GetImpl(context));
    *samples = count > std::numeric_limits<unsigned>::max()
        ? std::numeric_limits<unsigned>::max() : static_cast<unsigned>(count);
    return 1;
}
