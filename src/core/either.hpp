#include "either_impl.hpp"

namespace ropic
{
template <typename DATA, typename ERROR>
using Either = std::conditional_t<
    std::is_same_v<DATA, void>,
    detail::EitherImpl<Void, ERROR>,
    detail::EitherImpl<DATA, ERROR>>;
} // namespace ropic