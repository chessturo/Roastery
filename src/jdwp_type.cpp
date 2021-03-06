/* Defines C++ types to represent JDWP types
   Copyright 2021 Mitchell Levy

This file is a part of Roastery

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "jdwp_type.hpp"

#include <arpa/inet.h>

#include <algorithm>
#include <cstring>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

#include "jdwp_con.hpp"
#include "jdwp_packet.hpp"

namespace {

using namespace roastery;

/**
 * Convert a network byte order/big endian 64-bit value to host byte order.
 */
uint64_t ntohll(uint64_t network) {
#if __BIG_ENDIAN__
  return network;
#else
  uint32_t upper_bits = network >> 32;
  uint32_t lower_bits = network & UINT32_MAX;
  upper_bits = ntohl(upper_bits);
  lower_bits = ntohl(lower_bits);
  return (uint64_t{lower_bits} << 32) + upper_bits;
#endif
}

/**
 * Convert a host byte order 64-bit value to network byte order/big endian.
 */
uint64_t htonll(uint64_t host) {
#if __BIG_ENDIAN__
  return host;
#else
  uint32_t upper_bits = host >> 32;
  uint32_t lower_bits = host & UINT32_MAX;
  upper_bits = htonl(upper_bits);
  lower_bits = htonl(lower_bits);
  return (uint64_t{lower_bits} << 32) + upper_bits;
#endif
}

/**
 * Returns the size, in bytes of a JDWP entity based on the tag.
 *
 * @param t The tag whose size to get.
 * @param con The connection used to query the size for non-fixed width tags.
 *
 * @return The width of the data of a given tag.
 */
size_t GetSizeByTag(JdwpTag t, IJdwpCon& con) {
  switch(t) {
    case JdwpTag::kArray:
    case JdwpTag::kObject:
    case JdwpTag::kString:
    case JdwpTag::kThread:
    case JdwpTag::kThreadGroup:
    case JdwpTag::kClassLoader:
    case JdwpTag::kClassObject:
      return con.GetObjIdSize();
    case JdwpTag::kByte:
      return JdwpByte::value_size;
    case JdwpTag::kChar:
      return JdwpChar::value_size;
    case JdwpTag::kFloat:
      return JdwpFloat::value_size;
      break;
    case JdwpTag::kDouble:
      return JdwpDouble::value_size;
      break;
    case JdwpTag::kInt:
      return JdwpInt::value_size;
      break;
    case JdwpTag::kLong:
      return JdwpLong::value_size;
    case JdwpTag::kShort:
      return JdwpShort::value_size;
    case JdwpTag::kVoid:
      return 0;
    case JdwpTag::kBoolean:
      return JdwpBool::value_size;
    default:
      throw std::logic_error("Unknown tag");
  }
}

/**
 * Returns whether or not the given tag \c t is considered an object type.
 */
bool TagIsObjType(JdwpTag t) {
  return t == JdwpTag::kArray || t == JdwpTag::kObject ||
    t == JdwpTag::kString || t == JdwpTag::kThread ||
    t == JdwpTag::kThreadGroup || t == JdwpTag::kClassLoader ||
    t == JdwpTag::kClassObject;
}

}  // namespace

namespace roastery {

const string kNone = "No error has occurred.";
const string kInvalidThread = "Passed thread is null, is not a valid thread "
  "or has exited.";
const string kInvalidThreadGroup = "Thread group invalid.";
const string kInvalidPriority = "Invalid priority.";
const string kThreadNotSuspended = "If the specified thread has not been "
  "suspended by an event.";
const string kThreadSuspended = "Thread already suspended.";
const string kThreadNotAlive = "Thread has not been started or is now dead.";
const string kInvalidObject = "If this reference type has been unloaded and "
  "garbage collected.";
const string kInvalidClass = "Invalid class.";
const string kClassNotPrepared = "Class has been loaded but not yet prepared.";
const string kInvalidMethodId = "Invalid method.";
const string kInvalidLocation = "Invalid location.";
const string kInvalidFieldId = "Invalid field.";
const string kInvalidFrameId = "Invalid jframeID.";
const string kNoMoreFrames = "There are no more Java or JNI frames on the "
  "call stack.";
const string kOpaqueFrame= "Information about the frame is not available.";
const string kNotCurrentFrame = "Operation can only be performed on current "
  "frame.";
const string kTypeMismatch = "The variable is not an appropriate type for the "
  "function used.";
const string kInvalidSlot = "Invalid slot.";
const string kDuplicate = "Item already set.";
const string kNotFound = "Desired element not found.";
const string kInvalidMonitor = "Invalid monitor.";
const string kNotMonitorOwner = "This thread doesn't own the monitor.";
const string kInterrupt = "The call has been interrupted before completion.";
const string kInvalidClassFormat = "The virtual machine attempted to read a "
  "class file and determined that the file is malformed or otherwise cannot "
  "be interpreted as a class file.";
const string kCircularClassDefinition = "A circularity has been detected "
  "while initializing a class.";
const string kFailsVerification = "The verifier detected that a class file, "
  "though well formed, contained some sort of internal inconsistency or "
  "security problem.";
const string kAddMethodNotImplemented = "Adding methods has not been "
  "implemented.";
const string kSchemaChangeNotImplemented = "Schema change has not been "
  "implemented.";
const string kInvalidTypestate = "The state of the thread has been modified, "
  "and is now inconsistent.";
const string kHierarchyChangeNotImplemented = "A direct superclass is "
  "different for the new class version, or the set of directly implemented "
  "interfaces is different and canUnrestrictedlyRedefineClasses is false.";
const string kDeleteMethodNotImplemented = "The new class version does not "
  "declare a method declared in the old class version and "
  "canUnrestrictedlyRedefineClasses is false.";
const string kUnsupportedVersion = "A class file has a version number not "
  "supported by this VM.";
const string kNamesDontMatch = "The class name defined in the new class file "
  "is different from the name in the old class object.";
const string kClassModifiersChangeNotImplemented = "The new class version has "
  "different modifiers and and canUnrestrictedlyRedefineClasses is false.";
const string kMethodModifiersChangeNotImplemented = "A method in the new class "
  "version has different modifiers than its counterpart in the old class "
  "version and and canUnrestrictedlyRedefineClasses is false.";
const string kNotImplemented = "The functionality is not implemented in this "
  "virtual machine.";
const string kNullPointer = "Invalid pointer.";
const string kAbsentInformation = "Desired information is not available.";
const string kInvalidEventType = "The specified event type id is not "
  "recognized.";
const string kIllegalArgument = "Illegal argument.";
const string kOutOfMemory = "The function needed to allocate memory and no "
  "more memory was available for allocation.";
const string kAccessDenied = "Debugging has not been enabled in this virtual "
  "machine. JVMTI cannot be used.";
const string kVmDead = "The virtual machine is not running.";
const string kInternal = "An unexpected internal error has occurred.";
const string kUnattachedThread = "The thread being used to call this function "
  "is not attached to the virtual machine. Calls must be made from attached "
  "threads.";
const string kInvalidTag = "object type id or class tag.";
const string kAlreadyInvoking = "Previous invoke not complete.";
const string kInvalidIndex = "Index is invalid.";
const string kInvalidLength = "The length is invalid.";
const string kInvalidString = "The string is invalid.";
const string kInvalidClassLoader = "The class loader is invalid.";
const string kInvalidArray = "The array is invalid.";
const string kTransportLoad = "Unable to load the transport.";
const string kTransportInit = "Unable to initialize the transport.";
// The JDWP spec leaves this one blank
const string kNativeMethod = "NATIVE_METHOD error.";
const string kInvalidCount = "The count is invalid.";
const string kDefault = "Unknown error.";
const string& jdwp_strerror(JdwpError e) {
  switch(e) {
    case JdwpError::kNone:
      return kNone;
    case JdwpError::kInvalidThread:
      return kInvalidThread;
    case JdwpError::kInvalidThreadGroup:
      return kInvalidThreadGroup;
    case JdwpError::kInvalidPriority:
      return kInvalidPriority;
    case JdwpError::kThreadNotSuspended:
      return kThreadNotSuspended;
    case JdwpError::kThreadSuspended:
      return kThreadSuspended;
    case JdwpError::kThreadNotAlive:
      return kThreadNotAlive;
    case JdwpError::kInvalidObject:
      return kInvalidObject;
    case JdwpError::kInvalidClass:
      return kInvalidClass;
    case JdwpError::kClassNotPrepared:
      return kClassNotPrepared;
    case JdwpError::kInvalidMethodId:
      return kInvalidMethodId;
    case JdwpError::kInvalidLocation:
      return kInvalidLocation;
    case JdwpError::kInvalidFieldId:
      return kInvalidFieldId;
    case JdwpError::kInvalidFrameId:
      return kInvalidFrameId;
    case JdwpError::kNoMoreFrames:
      return kNoMoreFrames;
    case JdwpError::kOpaqueFrame:
      return kOpaqueFrame;
    case JdwpError::kNotCurrentFrame:
      return kNotCurrentFrame;
    case JdwpError::kTypeMismatch:
      return kTypeMismatch;
    case JdwpError::kInvalidSlot:
      return kInvalidSlot;
    case JdwpError::kDuplicate:
      return kDuplicate;
    case JdwpError::kNotFound:
      return kNotFound;
    case JdwpError::kInvalidMonitor:
      return kInvalidMonitor;
    case JdwpError::kNotMonitorOwner:
      return kNotMonitorOwner;
    case JdwpError::kInterrupt:
      return kInterrupt;
    case JdwpError::kInvalidClassFormat:
      return kInvalidClassFormat;
    case JdwpError::kCircularClassDefinition:
      return kCircularClassDefinition;
    case JdwpError::kFailsVerification:
      return kFailsVerification;
    case JdwpError::kAddMethodNotImplemented:
      return kAddMethodNotImplemented;
    case JdwpError::kSchemaChangeNotImplemented:
      return kSchemaChangeNotImplemented;
    case JdwpError::kInvalidTypestate:
      return kInvalidTypestate;
    case JdwpError::kHierarchyChangeNotImplemented:
      return kHierarchyChangeNotImplemented;
    case JdwpError::kDeleteMethodNotImplemented:
      return kDeleteMethodNotImplemented;
    case JdwpError::kUnsupportedVersion:
      return kUnsupportedVersion;
    case JdwpError::kNamesDontMatch:
      return kNamesDontMatch;
    case JdwpError::kClassModifiersChangeNotImplemented:
      return kClassModifiersChangeNotImplemented;
    case JdwpError::kMethodModifiersChangeNotImplemented:
      return kMethodModifiersChangeNotImplemented;
    case JdwpError::kNotImplemented:
      return kNotImplemented;
    case JdwpError::kNullPointer:
      return kNullPointer;
    case JdwpError::kAbsentInformation:
      return kAbsentInformation;
    case JdwpError::kInvalidEventType:
      return kInvalidEventType;
    case JdwpError::kIllegalArgument:
      return kIllegalArgument;
    case JdwpError::kOutOfMemory:
      return kOutOfMemory;
    case JdwpError::kAccessDenied:
      return kAccessDenied;
    case JdwpError::kVmDead:
      return kVmDead;
    case JdwpError::kInternal:
      return kInternal;
    case JdwpError::kUnattachedThread:
      return kUnattachedThread;
    case JdwpError::kInvalidTag:
      return kInvalidTag;
    case JdwpError::kAlreadyInvoking:
      return kAlreadyInvoking;
    case JdwpError::kInvalidIndex:
      return kInvalidIndex;
    case JdwpError::kInvalidLength:
      return kInvalidLength;
    case JdwpError::kInvalidString:
      return kInvalidString;
    case JdwpError::kInvalidClassLoader:
      return kInvalidClassLoader;
    case JdwpError::kInvalidArray:
      return kInvalidArray;
    case JdwpError::kTransportLoad:
      return kTransportLoad;
    case JdwpError::kTransportInit:
      return kTransportInit;
    case JdwpError::kNativeMethod:
      return kNativeMethod;
    case JdwpError::kInvalidCount:
      return kInvalidCount;
    default:
      return kDefault;
  }
}

string IJdwpField::Serialize(IJdwpCon& con) const {
  return this->SerializeImpl(con);
}
size_t IJdwpField::FromEncoded(const string& encoded, IJdwpCon& con) {
  return this->FromEncodedImpl(encoded, con);
}
// pure virtual dtor housekeeping
IJdwpField::~IJdwpField() { }

JdwpTaggedObjectId::JdwpTaggedObjectId() = default;

size_t JdwpTaggedObjectId::FromEncodedImpl(const string& data, IJdwpCon& con) {
  const char* data_arr = data.data();
  this->tag = static_cast<JdwpTag>(data_arr[0]);

  size_t obj_id_size = this->obj_id.FromEncoded(&data_arr[1], con);
  return 1 + obj_id_size;
}

JdwpTaggedObjectId::JdwpTaggedObjectId(JdwpTag tag, JdwpObjId obj_id)
    : tag(tag), obj_id(obj_id) { }

string JdwpTaggedObjectId::SerializeImpl(IJdwpCon& con) const {
  string out = { static_cast<char>(this->tag) };
  out += this->obj_id.Serialize(con);
  return out;
}

JdwpLocation::JdwpLocation() = default;

JdwpLocation::JdwpLocation(JdwpTypeTag type, JdwpClassId class_id,
    JdwpMethodId method_id, uint64_t index) : type(type), class_id(class_id),
    method_id(method_id), index(index) { }

size_t JdwpLocation::FromEncodedImpl(const string& encoded, IJdwpCon& con) {
  size_t total_size = 0;
  const char* encoded_bytes = encoded.data();
  this->type = static_cast<JdwpTypeTag>(encoded_bytes[0]);
  total_size += 1;  // For JdwpTypeTag

  size_t obj_id_size = con.GetObjIdSize();
  size_t method_id_size = con.GetMethodIdSize();
  total_size += this->class_id.FromEncoded(
      string(&encoded_bytes[1], obj_id_size), con);
  total_size += this->method_id.FromEncoded(
      string(&encoded_bytes[1 + obj_id_size], method_id_size), con);

  uint64_t index_network_byte_order =
    *reinterpret_cast<const uint64_t*>(
        &encoded_bytes[1 + obj_id_size + method_id_size]);
  this->index = ntohll(index_network_byte_order);
  total_size += sizeof(uint64_t);  // For location index

  return total_size;
}

string JdwpLocation::SerializeImpl(IJdwpCon& con) const {
  string res = { static_cast<char>(this->type) };
  res.append(this->class_id.Serialize(con));
  res.append(this->method_id.Serialize(con));

  uint64_t index_network_byte_order = htonll(this->index);
  res.append(string(reinterpret_cast<char*>(&index_network_byte_order),
        sizeof(index_network_byte_order)));

  return res;
}

JdwpString::JdwpString() = default;

JdwpString& JdwpString::operator<<(const string& s) {
  this->data = s;
  return *this;
}

string& JdwpString::GetValue() {
  return this->data;
}

const string& JdwpString::GetValue() const {
  return this->data;
}

size_t JdwpString::FromEncodedImpl(const string& data, IJdwpCon& con) {
  static_cast<void>(con);  // non variable-width type
  const char* data_bytes = data.data();
  uint32_t strlen = ntohl(*reinterpret_cast<const uint32_t*>(data_bytes));
  this->data = (string(data_bytes + 4, strlen));

  return sizeof(uint32_t) + strlen;  // uint32_t for size, strlen for the string
}

string JdwpString::SerializeImpl(IJdwpCon& con) const {
  static_cast<void>(con);  // non variable-width type
  uint32_t len_network_byte_order = htonl(data.length());
  string res = string(reinterpret_cast<char*>(&len_network_byte_order),
      sizeof(len_network_byte_order));
  res += data;
  return res;
}

JdwpString::JdwpString(const string& data) : data(data) { }

JdwpValue::JdwpValue() = default;

JdwpValue::JdwpValue(JdwpTag tag, JdwpVal val) : tag(tag), value(val) { }

size_t JdwpValue::FromEncodedAsUntagged(JdwpTag t, const string& encoded,
    IJdwpCon& con) {
  this->tag = t;
  if (t == JdwpTag::kVoid) {
    // Do this case seperately because we don't want to deal with trying to read
    // from the string
    this->value = VoidValue();
    return JdwpValue::VoidValue::value_size;
  } else {
    if (TagIsObjType(t)) {
      this->value = JdwpObjId();
      std::visit([&](auto&& s) { s.FromEncoded(encoded, con); }, this->value);
      return con.GetObjIdSize();
    } else {
      switch(t) {
        case JdwpTag::kBoolean:
          this->value = JdwpBool();
          break;
        case JdwpTag::kByte:
          this->value = JdwpByte();
          break;
        case JdwpTag::kChar:
          this->value = JdwpChar();
          break;
        case JdwpTag::kFloat:
          this->value = JdwpFloat();
          break;
        case JdwpTag::kDouble:
          this->value = JdwpDouble();
          break;
        case JdwpTag::kInt:
          this->value = JdwpInt();
          break;
        case JdwpTag::kLong:
          this->value = JdwpLong();
          break;
        case JdwpTag::kShort:
          this->value = JdwpShort();
          break;
        default:
          throw JdwpException("Unknown Tag");
      }
      size_t val_size;
      std::visit([&](auto&& s) {
        s.FromEncoded(encoded, con);
        val_size = s.value_size;
      }, this->value);
      return val_size;
    }
  }
}

string JdwpValue::SerializeAsUntagged(IJdwpCon& con) const {
  string res;
  std::visit([&res, &con](auto&& v) {
    res = v.Serialize(con);
  }, this->value);
  return res;
}

string JdwpValue::SerializeImpl(IJdwpCon& con) const {
  return static_cast<char>(tag) + this->SerializeAsUntagged(con);
}

size_t JdwpValue::FromEncodedImpl(const string& encoded, IJdwpCon& con) {
  this->tag = static_cast<JdwpTag>(encoded[0]);
  size_t untagged_size =
    this->FromEncodedAsUntagged(this->tag, encoded.substr(1), con);
  return 1 + untagged_size;
}

JdwpArrayRegion::JdwpArrayRegion() = default;

JdwpArrayRegion::JdwpArrayRegion(JdwpTag tag,
  const vector<unique_ptr<JdwpValue>>& values) :
    tag(tag),
    values(unique_ptr<vector<unique_ptr<JdwpValue>>>(
          new vector<unique_ptr<JdwpValue>>(values.size()))) {
  for (auto it = values.begin(); it != values.end(); ++it) {
    unique_ptr<JdwpValue> copy =
      unique_ptr<JdwpValue>(new JdwpValue(it->get()->tag, it->get()->value));
    this->values->push_back(move(copy));
  }
}

size_t JdwpArrayRegion::FromEncodedImpl(const string& encoded, IJdwpCon& con) {
  tag = static_cast<JdwpTag>(encoded[0]);
  if (!values) {
    values = unique_ptr<vector<unique_ptr<JdwpValue>>>(
        new vector<unique_ptr<JdwpValue>>());
  } else {
    values->clear();
  }

  uint32_t value_count =
    ntohl(*reinterpret_cast<const uint32_t*>(encoded.data() + 1));
  size_t value_size = GetSizeByTag(this->tag, con);

  const size_t kHeaderOffset = 5; // 1 for the tag, 4 for the length
  // Object types in array regions are handled differently
  if (TagIsObjType(this->tag)) {
    // Object types are tagged values, so they have an extra byte
    value_size++;
    for (size_t i = kHeaderOffset;
        i < kHeaderOffset + value_size * value_count;
        i += value_size) {
      unique_ptr<JdwpValue> val = unique_ptr<JdwpValue>(new JdwpValue());
      val->FromEncoded(encoded.substr(i, i + value_size), con);
      this->values->push_back(std::move(val));
    }
  } else {
    for (size_t i = kHeaderOffset;
        i < kHeaderOffset + value_size * value_count;
        i += value_size) {
      unique_ptr<JdwpValue> val = unique_ptr<JdwpValue>(new JdwpValue());
      val->FromEncodedAsUntagged(this->tag, encoded.substr(i, value_size), con);
      this->values->push_back(std::move(val));
    }
  }

  return sizeof(JdwpTag) + sizeof(value_count) + (value_count * value_size);
}

string JdwpArrayRegion::SerializeImpl(IJdwpCon& con) const {
  std::stringstream res;
  res << static_cast<char>(this->tag);
  uint32_t len_nbo = htonl(this->values->size());
  res.write(reinterpret_cast<char*>(&len_nbo), sizeof(len_nbo));
  for (auto it = this->values->begin(); it != this->values->end(); ++it) {
    if (TagIsObjType(this->tag)) {
      res << it->get()->Serialize(con);
    } else {
      res << it->get()->SerializeAsUntagged(con);
    }
  }
  return res.str();
}

}  // namespace roastery

