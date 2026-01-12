/**
 * @file Config.h
 * @brief Configuration structure for SystemChrono.
 *
 * SystemChrono provides time utility functions and timer classes. This Config
 * struct is provided for consistency with the template pattern but is not
 * required for basic usage of the free functions and elapsed timer classes.
 */

#pragma once

#include <stdint.h>

namespace SystemChrono {

/**
 * @brief Configuration for SystemChrono (reserved for future use).
 *
 * SystemChrono provides free functions and timer classes that work without
 * configuration. This struct exists for API consistency and future extensions.
 *
 * @note Currently unused. All SystemChrono functions work out of the box.
 * @note begin() accepts Config for future compatibility.
 */
struct Config {
  // Reserved for future configuration options
};

}  // namespace SystemChrono
