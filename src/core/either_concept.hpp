
#include <variant>

namespace ropic::detail
{
/// @brief Ensures a type is a plain value: not a reference, not
/// const-qualified, not void, and not std::monostate.
template <typename T>
concept plain_value_type = std::same_as<T, std::remove_cvref_t<T>>
                        && !std::same_as<T, std::monostate>
                        && !std::is_void_v<T>;

template <typename DATA, typename ERROR>
concept either_concept = plain_value_type<DATA>
                      && plain_value_type<ERROR>
                      && !std::is_same_v<DATA, ERROR>;

} // namespace ropic::detail
