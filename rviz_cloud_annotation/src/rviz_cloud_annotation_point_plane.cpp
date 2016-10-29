/*
 * Copyright (c) 2016, Riccardo Monica
 */

#include "rviz_cloud_annotation_point_plane.h"

#include <cmath>

RVizCloudAnnotationPointsPointPlane::RVizCloudAnnotationPointsPointPlane(const uint64 cloud_size,
  const PointNeighborhood & point_neighborhood,const float multiplier): m_point_neighborhood(point_neighborhood)
{
  m_multiplier = multiplier;
  m_cloud_size = cloud_size;

  m_labels_assoc.resize(cloud_size,0);
  m_last_generated_tot_dists.resize(cloud_size,0.0);
}

void RVizCloudAnnotationPointsPointPlane::Clear()
{
  m_labels_assoc.assign(m_cloud_size,0);
  m_last_generated_tot_dists.assign(m_cloud_size,0.0);
}

void RVizCloudAnnotationPointsPointPlane::RemoveLabel(const uint64 label_id,
                                                      BoolVector & touched_labels,
                                                      BoolVector & touched_points)
{
  Uint64Set seeds_set;

  for (uint64 i = 0; i < m_cloud_size; i++)
  {
    if (m_labels_assoc[i] == label_id)
    {
      const float * neigh_dists;
      const float * neigh_tot_dists;
      const uint64 * neighs;
      const uint64 neighs_size = m_point_neighborhood.GetNeigborhoodAsPointer(i,neighs,neigh_tot_dists,neigh_dists);

      for (uint64 h = 0; h < neighs_size; h++)
      {
        const uint32 label = m_labels_assoc[neighs[h]];
        if (label != 0 && label != label_id)
          seeds_set.insert(neighs[h]);
      }

      m_labels_assoc[i] = 0;
      m_last_generated_tot_dists[i] = 0.0;
      touched_points[i] = true;
    }
  }

  const Uint64Vector seeds(seeds_set.begin(),seeds_set.end());

  UpdateRegionGrowing(seeds,touched_labels,touched_points);
  touched_labels[label_id - 1] = true;
}

void RVizCloudAnnotationPointsPointPlane::UpdateRegionGrowing(const Uint64Vector & seeds,
                                                              BoolVector & touched_labels,
                                                              BoolVector & touched_points)
{
  touched_points.resize(m_cloud_size,false);

  Uint64Queue queue;
  BoolVector in_queue(m_cloud_size,false);
  for (uint64 i = 0; i < seeds.size(); i++)
  {
    const uint64 first = seeds[i];
    queue.push(first);
    in_queue[first] = true;
    touched_points[first] = true;
  }

  if (!std::isfinite(m_multiplier))
    return; // do not expand if multiplier is infinite (i.e. weight is zero)

  while (!queue.empty())
  {
    const uint64 current = queue.front();
    queue.pop();
    in_queue[current] = false;

    const float current_tot_dist = m_last_generated_tot_dists[current];
    const uint64 current_label = m_labels_assoc[current];

    const float * neigh_dists;
    const float * neigh_tot_dists;
    const uint64 * neighs;
    const uint64 neighs_size = m_point_neighborhood.GetNeigborhoodAsPointer(current,neighs,neigh_tot_dists,neigh_dists);

    for (uint64 i = 0; i < neighs_size; i++)
    {
      const uint64 next = neighs[i];
      const float neigh_tot_dist = neigh_tot_dists[i] * m_multiplier;
      const float next_tot_dist = neigh_tot_dist + current_tot_dist;
      const uint64 next_label = m_labels_assoc[next];

      if (next_tot_dist > 1.0)
        continue;

      if (next_label != 0 && m_last_generated_tot_dists[next] <= next_tot_dist)
        continue;

      if (next_label != 0)
        touched_labels[next_label - 1] = true;
      touched_labels[current_label - 1] = true;

      m_last_generated_tot_dists[next] = next_tot_dist;
      m_labels_assoc[next] = current_label;
      touched_points[next] = true;

      if (!in_queue[next])
      {
        in_queue[next] = true;
        queue.push(next);
      }
    }
  }
}

void RVizCloudAnnotationPointsPointPlane::SetSeed(const uint64 point_id,const uint32 label_id)
{
  m_labels_assoc[point_id] = label_id;
  m_last_generated_tot_dists[point_id] = 0.0;
}