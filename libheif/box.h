/*
 * H.265 video codec.
 * Copyright (c) 2013-2017 struktur AG, Dirk Farin <farin@struktur.de>
 *
 * This file is part of libde265.
 *
 * libde265 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * libde265 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libde265.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBHEIF_BOX_H
#define LIBHEIF_BOX_H

#include <inttypes.h>
#include <vector>
#include <string>
#include <memory>
#include <limits>
#include <istream>


namespace heif {

  class Error
  {
  public:
    enum ErrorCode {
      Ok,
      ParseError,
      EndOfData
    } error_code;


    Error() : error_code(Ok) { }
    Error(ErrorCode c) : error_code(c) { }

    static Error OK;
  };


  class BitstreamRange
  {
  public:
    BitstreamRange(std::istream* istr, uint64_t length, BitstreamRange* parent = nullptr) {
      m_remaining = length;
      m_end_reached = (length==0);

      m_istr = istr;
      m_parent_range = parent;
    }

    bool read(int n) {
      if (m_remaining>=n) {
        if (m_parent_range) {
          m_parent_range->read(n);
        }

        m_remaining -= n;
        m_end_reached = (m_remaining==0);

        return true;
      }
      else if (m_remaining==0) {
        m_error = true;
        return false;
      }
      else {
        if (m_parent_range) {
          m_parent_range->read(m_remaining);
        }

        m_istr->seekg(m_remaining, std::ios::cur);
        m_remaining = 0;
        m_end_reached = true;
        m_error = true;
        return false;
      }
    }

    void skip_to_end_of_file() {
      m_istr->seekg(0, std::ios_base::end);
      m_remaining = 0;
      m_end_reached = true;
    }

    void skip_to_end_of_box() {
      if (m_remaining) {
        m_istr->seekg(m_remaining, std::ios_base::cur);
        m_remaining = 0;
      }

      m_end_reached = true;
    }

    void set_eof_reached() {
      m_remaining = 0;
      m_end_reached = true;

      if (m_parent_range) {
        m_parent_range->set_eof_reached();
      }
    }

    bool eof() const {
      return m_end_reached;
    }

    bool error() const {
      return m_error;
    }

    Error get_error() const {
      if (m_error) {
        return Error(Error::EndOfData);
      }
      else {
        return Error::OK;
      }
    }

    std::istream* get_istream() { return m_istr; }

  private:
    std::istream* m_istr = nullptr;
    BitstreamRange* m_parent_range = nullptr;

    uint64_t m_remaining;
    bool m_end_reached = false;
    bool m_error = false;
  };


  class BoxHeader {
  public:
    BoxHeader();
    ~BoxHeader() { }

    constexpr static uint64_t size_until_end_of_file = 0;

    uint64_t get_box_size() const { return m_size; }

    uint32_t get_header_size() const { return m_header_size; }

    uint32_t get_short_type() const { return m_type; }

    std::vector<uint8_t> get_type() const;

    std::string get_type_string() const;

    Error parse(BitstreamRange& range);

    Error write(std::ostream& ostr) const;

    std::string dump() const;


    // --- full box

    Error parse_full_box_header(BitstreamRange& range);

    uint8_t get_version() const { return m_version; }

  private:
    uint64_t m_size = 0;
    uint32_t m_header_size = 0;

    uint32_t m_type = 0;
    std::vector<uint8_t> m_uuid_type;


    bool m_is_full_box = false;

    uint8_t m_version = 0;
    uint32_t m_flags = 0;
  };



  class Box : public BoxHeader {
  public:
    Box(const BoxHeader& hdr) : BoxHeader(hdr) { }
    virtual ~Box() { }

    static std::shared_ptr<Box> read(BitstreamRange& range);

    virtual Error write(std::ostream& ostr) const { return Error::OK; }

    virtual std::string dump() const;

  protected:
    virtual Error parse(BitstreamRange& range);

    std::vector<std::shared_ptr<Box>> m_children;

    Error read_children(BitstreamRange& range);

    std::string dumpChildren() const;
  };


  /*
  class BoxFull : public Box {
  public:
  BoxFull(const BoxHeader& hdr) : Box(hdr) { }

    std::string dump() const override;

  protected:
    Error parse(BitstreamRange& range) override;

  private:
  };
*/

  class Box_ftyp : public Box {
  public:
  Box_ftyp(const BoxHeader& hdr) : Box(hdr) { }

    std::string dump() const override;

  protected:
    Error parse(BitstreamRange& range);

  private:
    uint32_t m_major_brand;
    uint32_t m_minor_version;
    std::vector<uint32_t> m_compatible_brands;
  };


  class Box_meta : public Box {
  public:
  Box_meta(const BoxHeader& hdr) : Box(hdr) { }

    std::string dump() const override;

  protected:
    Error parse(BitstreamRange& range);
  };


  class Box_hdlr : public Box {
  public:
  Box_hdlr(const BoxHeader& hdr) : Box(hdr) { }

    std::string dump() const override;

  protected:
    Error parse(BitstreamRange& range);

  private:
    uint32_t m_pre_defined;
    uint32_t m_handler_type;
    uint32_t m_reserved[3];
    std::string m_name;
  };


  class Box_pitm : public Box {
  public:
  Box_pitm(const BoxHeader& hdr) : Box(hdr) { }

    std::string dump() const override;

  protected:
    Error parse(BitstreamRange& range);

  private:
    uint16_t m_item_ID;
  };


  class Box_iloc : public Box {
  public:
  Box_iloc(const BoxHeader& hdr) : Box(hdr) { }

    std::string dump() const override;

  protected:
    Error parse(BitstreamRange& range);

  private:
    uint16_t m_item_ID;

    struct Extent {
      uint64_t offset;
      uint64_t length;
    };

    struct Item {
      uint16_t item_ID;
      uint16_t data_reference_index;
      uint64_t base_offset;

      std::vector<Extent> extents;
    };

    std::vector<Item> m_items;
  };


  class Box_infe : public Box {
  public:
  Box_infe(const BoxHeader& hdr) : Box(hdr) { }

    std::string dump() const override;

  protected:
    Error parse(BitstreamRange& range);

  private:
      uint16_t m_item_ID;
      uint16_t m_item_protection_index;

      std::string m_item_type;
      std::string m_item_name;
      std::string m_content_type;
      std::string m_content_encoding;
      std::string m_item_uri_type;
    };


  class Box_iinf : public Box {
  public:
  Box_iinf(const BoxHeader& hdr) : Box(hdr) { }

    std::string dump() const override;

  protected:
    Error parse(BitstreamRange& range);

  private:
    //std::vector< std::shared_ptr<Box_infe> > m_iteminfos;
  };


  class Box_iprp : public Box {
  public:
  Box_iprp(const BoxHeader& hdr) : Box(hdr) { }

    std::string dump() const override;

  protected:
    Error parse(BitstreamRange& range);
  };


  class Box_ipco : public Box {
  public:
  Box_ipco(const BoxHeader& hdr) : Box(hdr) { }

    std::string dump() const override;

  protected:
    Error parse(BitstreamRange& range);
  };


  class Box_ispe : public Box {
  public:
  Box_ispe(const BoxHeader& hdr) : Box(hdr) { }

    std::string dump() const override;

  protected:
    Error parse(BitstreamRange& range);

  private:
    uint32_t m_image_width;
    uint32_t m_image_height;
  };


  class Box_hvcC : public Box {
  public:
  Box_hvcC(const BoxHeader& hdr) : Box(hdr) { }

    std::string dump() const override;

  protected:
    Error parse(BitstreamRange& range);

  private:
    uint8_t  m_configuration_version;
    uint8_t  m_general_profile_space;
    bool     m_general_tier_flag;
    uint8_t  m_general_profile_idc;
    uint32_t m_general_profile_compatibility_flags;
    std::array<bool,48> m_general_constraint_indicator_flags;
    uint8_t  m_general_level_idc;

    uint16_t m_min_spatial_segmentation_idc;
    uint8_t  m_parallelism_type;
    uint8_t  m_chroma_format;
    uint8_t  m_bit_depth_luma;
    uint8_t  m_bit_depth_chroma;
    uint16_t m_avg_frame_rate;

    uint8_t  m_constant_frame_rate;
    uint8_t  m_num_temporal_layers;
    uint8_t  m_temporal_id_nested;
    uint8_t  m_length_size;

    struct NalArray {
      uint8_t m_array_completeness;
      uint8_t m_NAL_unit_type;

      std::vector< std::vector<uint8_t> > m_nal_units;
    };

    std::vector<NalArray> m_nal_array;
  };
}

#endif
