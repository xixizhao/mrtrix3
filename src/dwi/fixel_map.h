/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2012.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/



#ifndef __dwi_fixel_map_h__
#define __dwi_fixel_map_h__


#include "image/buffer_scratch.h"
#include "image/loop.h"
#include "image/nav.h"
#include "image/voxel.h"

#include "dwi/fmls.h"
#include "dwi/directions/mask.h"


namespace MR
{
  namespace DWI
  {



    template <class Fixel>
    class Fixel_map
    {

      public:
        template <typename Set>
        Fixel_map (const Set& i) :
        info_ (i),
        data  (info_, "fixel map voxels"),
        accessor (data)
        {
          Image::Voxel< Image::BufferScratch<MapVoxel*> > v (data);
          Image::Loop loop;
          for (loop.start (v); loop.ok(); loop.next (v))
            v.value() = NULL;
          // fixels[0] is an invaid fixel, as provided by the relevant empty constructor
          // This allows index 0 to be used as an error code, simplifying the implementation of MapVoxel and Iterator
          fixels.push_back (Fixel());
        }

        class MapVoxel;
        typedef Image::Voxel< Image::BufferScratch<MapVoxel*> > VoxelAccessor;

        virtual ~Fixel_map()
        {
          Image::Loop loop;
          VoxelAccessor v (accessor);
          for (loop.start (v); loop.ok(); loop.next (v)) {
            if (v.value()) {
              delete v.value();
              v.value() = NULL;
            }
          }
        }


        class Iterator;
        class ConstIterator;

        Iterator      begin (VoxelAccessor& v)       { return Iterator      (v.value(), *this); }
        ConstIterator begin (VoxelAccessor& v) const { return ConstIterator (v.value(), *this); }

        Fixel&       operator[] (const size_t i)       { return fixels[i]; }
        const Fixel& operator[] (const size_t i) const { return fixels[i]; }

        virtual bool operator() (const FMLS::FOD_lobes& in);

        const Image::Info& info() const { return info_; }


      protected:

        const class Info : public Image::Info {
          public:
          template <typename T>
          Info (const T& i) :
          Image::Info (i.info())
          {
            set_ndim (3);
          }
          Info () : Image::Info () { }
        } info_;

        Image::BufferScratch<MapVoxel*> data;
        const VoxelAccessor accessor; // Functions can copy-construct their own voxel accessor from this and retain const-ness
        std::vector<Fixel> fixels;


        Fixel_map (const Fixel_map& that) : info_ (that.data), data (info_) { assert (0); }


    };




    template <class Fixel>
    class Fixel_map<Fixel>::MapVoxel
    {
      public:
        MapVoxel (const FMLS::FOD_lobes& in, const size_t first) :
          first_fixel_index (first),
          count (in.size()),
          lookup_table (new uint8_t[in.lut.size()])
        {
          memcpy (lookup_table, &in.lut[0], in.lut.size() * sizeof (uint8_t));
        }

        MapVoxel (const size_t first, const size_t size) :
          first_fixel_index (first),
          count (size),
          lookup_table (NULL) { }

        ~MapVoxel()
        {
          if (lookup_table) {
            delete[] lookup_table;
            lookup_table = NULL;
          }
        }

        size_t first_index() const { return first_fixel_index; }
        size_t num_fixels()  const { return count; }
        bool   empty()       const { return !count; }

        // Direction must have been assigned to a histogram bin first
        size_t dir2fixel (const size_t dir) const
        {
          assert (lookup_table);
          const size_t offset = lookup_table[dir];
          return ((offset == count) ? 0 : (first_fixel_index + offset));
        }


      private:
        size_t first_fixel_index, count;
        uint8_t* lookup_table;

    };



    template <class Fixel>
    class Fixel_map<Fixel>::Iterator
    {
        friend class Fixel_map<Fixel>::ConstIterator;
      public:
        Iterator (const MapVoxel* const voxel, Fixel_map<Fixel>& parent) :
          index (voxel ? voxel->first_index() : 0),
          last  (voxel ? (index + voxel->num_fixels()) : 0),
          fixel_map (parent) { }
        Iterator& operator++ ()       { ++index; return *this; }
        Fixel&    operator() () const { return fixel_map.fixels[index]; }
        operator  bool ()       const { return (index != last); }
        operator  size_t ()     const { return index; }
      private:
        size_t index, last;
        Fixel_map<Fixel>& fixel_map;
    };

    template <class Fixel>
    class Fixel_map<Fixel>::ConstIterator
    {
      public:
        ConstIterator (const MapVoxel* const voxel, const Fixel_map& parent) :
          index   (voxel ? voxel->first_index() : 0),
          last    (voxel ? (index + voxel->num_fixels()) : 0),
          fixel_map (parent) { }
        ConstIterator (const Iterator& that) :
          index   (that.index),
          last    (that.last),
          fixel_map (that.fixel_map) { }
        ConstIterator&  operator++ ()       { ++index; return *this; }
        const Fixel&    operator() () const { return fixel_map.fixels[index]; }
        operator        bool ()       const { return (index != last); }
        operator        size_t ()     const { return index; }
      private:
        size_t index, last;
        const Fixel_map<Fixel>& fixel_map;
    };




    template <class Fixel>
    bool Fixel_map<Fixel>::operator() (const FMLS::FOD_lobes& in)
    {
        if (in.empty())
          return true;
        if (!Image::Nav::within_bounds (data, in.vox))
          return false;
        VoxelAccessor v (accessor);
        Image::Nav::set_pos (v, in.vox);
        if (v.value())
          throw Exception ("FIXME: FOD_map has received multiple segmentations for the same voxel!");
        v.value() = new MapVoxel (in, fixels.size());
        for (FMLS::FOD_lobes::const_iterator i = in.begin(); i != in.end(); ++i)
          fixels.push_back (Fixel (*i));
        return true;
    }




  }
}



#endif
