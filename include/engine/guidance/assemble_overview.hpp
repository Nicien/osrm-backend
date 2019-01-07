#ifndef ENGINE_GUIDANCE_ASSEMBLE_OVERVIEW_HPP
#define ENGINE_GUIDANCE_ASSEMBLE_OVERVIEW_HPP

#include "engine/guidance/leg_geometry.hpp"

#include "util/coordinate.hpp"

#include <vector>

namespace osrm
{
namespace engine
{
namespace guidance
{

struct Overview {
    std::vector<util::Coordinate> geometry;
    std::vector<size_t> legs_indices;
};

Overview assembleOverview(const std::vector<LegGeometry> &leg_geometries,
                                               const bool use_simplification);

} // namespace guidance
} // namespace engine
} // namespace osrm

#endif
