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
 * Reads a \c JdwpObjId from the JDWP encoded version.
 *
 * @param data A pointer to the first byte of the encoded object ID.
 * @param con The connection the data was received on. Used to get needed
 * deserialization info.
 */
JdwpObjId DeserializeObjId(const char* data, IJdwpCon& con) {
  // Zero all bytes of `res`
  JdwpObjId res = 0;

  size_t obj_id_size = con.GetObjIdSize();
  memcpy(&res, data, obj_id_size);

  res = ntohll(res);
  return res;
}

/**
 * Reads a \c JdwpMethodId from the JDWP encoded version.
 *
 * @param data A pointer to the first byte of the encoded object ID.
 * @param con The connection the data was received on. Used to get needed
 * deserialization info.
 */
JdwpObjId DeserializeMethodId(const char* data, IJdwpCon& con) {
  // Zero all bytes of `res`
  JdwpObjId res = 0;

  size_t method_id_size = con.GetMethodIdSize();
  memcpy(&res, data, method_id_size);

  res = ntohll(res);
  return res;
}

/**
 * Encodes a \c JdwpObjId for transport over the given \c JdwpCon.
 *
 * @param data The \c JdwpObjId to serialize.
 * @param con The connection the returned value will be sent over. Used to get
 * needed serialization info.
 */
string SerializeObjId(JdwpObjId data, IJdwpCon& con) {
  uint64_t data_network = htonll(data);
  size_t obj_id_size = con.GetObjIdSize();
  // If we're sending a big-endian n byte number where n < 8, we don't want to
  // start writing from the lowest address, we want to start writing from the
  // (8 - n)th address.
  int byte_offset = sizeof(JdwpObjId) - obj_id_size;

  return string(reinterpret_cast<char*>(&data_network) + byte_offset,
      obj_id_size);

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
  size_t len;
  switch(t) {
    case JdwpTag::kArray:
    case JdwpTag::kObject:
    case JdwpTag::kString:
    case JdwpTag::kThread:
    case JdwpTag::kThreadGroup:
    case JdwpTag::kClassLoader:
    case JdwpTag::kClassObject:
      len = con.GetObjIdSize();
      break;
    case JdwpTag::kByte:
      len = sizeof(JdwpByte);
      break;
    case JdwpTag::kChar:
      len = sizeof(JdwpChar);
      break;
    case JdwpTag::kFloat:
      len = sizeof(JdwpFloat);
      break;
    case JdwpTag::kDouble:
      len = sizeof(JdwpDouble);
      break;
    case JdwpTag::kInt:
      len = sizeof(JdwpInt);
      break;
    case JdwpTag::kLong:
      len = sizeof(JdwpLong);
      break;
    case JdwpTag::kShort:
      len = sizeof(JdwpShort);
      break;
    case JdwpTag::kVoid:
      len = 0;
      break;
    case JdwpTag::kBoolean:
      len = sizeof(JdwpBool);
      break;
    default:
      throw std::logic_error("Unknown tag");
  }
  return len;
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

JdwpTaggedObjectId::JdwpTaggedObjectId() = default;

JdwpTaggedObjectId::JdwpTaggedObjectId(const string& data, IJdwpCon& con) {
  const char* data_arr = data.data();
  this->tag = static_cast<JdwpTag>(data_arr[0]);

  this->obj_id = DeserializeObjId(&data_arr[1], con);
}

JdwpTaggedObjectId::JdwpTaggedObjectId(JdwpTag tag, JdwpObjId obj_id)
    : tag(tag), obj_id(obj_id) { }

string JdwpTaggedObjectId::Serialize(IJdwpCon& con) const {
  string out = { static_cast<char>(this->tag) };
  out += SerializeObjId(this->obj_id, con);
  return out;
}

JdwpLocation::JdwpLocation() = default;

JdwpLocation::JdwpLocation(JdwpTypeTag type, JdwpClassId class_id,
    JdwpMethodId method_id, uint64_t index) : type(type), class_id(class_id),
    method_id(method_id), index(index) { }

JdwpLocation::JdwpLocation(const string& encoded, IJdwpCon& con) {
  const char* encoded_bytes = encoded.data();
  this->type = static_cast<JdwpTypeTag>(encoded_bytes[0]);

  size_t obj_id_size = con.GetObjIdSize();
  size_t method_id_size = con.GetMethodIdSize();
  this->class_id = DeserializeObjId(&encoded_bytes[1], con);
  this->method_id = DeserializeMethodId(&encoded_bytes[1 + obj_id_size], con);

  uint64_t index_network_byte_order = 
    *reinterpret_cast<const uint64_t*>(
        &encoded_bytes[1 + obj_id_size + method_id_size]);
  this->index = ntohll(index_network_byte_order);
}

string JdwpLocation::Serialize(IJdwpCon& con) const {
  string res = { static_cast<char>(this->type) };
  res.append(SerializeObjId(this->class_id, con));
  res.append(SerializeObjId(this->method_id, con));

  uint64_t index_network_byte_order = htonll(this->index);
  res.append(string(reinterpret_cast<char*>(&index_network_byte_order),
        sizeof(index_network_byte_order)));

  return res;
}

JdwpString::JdwpString() = default;

JdwpString JdwpString::fromSerialized(const string& data) {
  const char* data_bytes = data.data();
  uint32_t strlen = ntohl(*reinterpret_cast<const uint32_t*>(data_bytes));
  return JdwpString(string(data_bytes + 4, strlen));
}

JdwpString JdwpString::fromHost(const string& data) {
  return JdwpString(data);
}

string JdwpString::Serialize() const {
  uint32_t len_network_byte_order = htonl(data.length());
  string res = string(reinterpret_cast<char*>(&len_network_byte_order),
      sizeof(len_network_byte_order));
  res += data;
  return res;
}

JdwpString::JdwpString(const string& data) : data(data) { }

JdwpValue::JdwpValue() = default;

JdwpValue::JdwpValue(JdwpTag tag, JdwpValUnion val) : tag(tag), value(val) { }

JdwpValue::JdwpValue(IJdwpCon& con, const string& encoded) :
    tag(static_cast<JdwpTag>(encoded[0])) {
  size_t len = GetSizeByTag(this->tag, con);
  string data = encoded.substr(1, len);
  reverse(data.begin(), data.end());
  memcpy(&this->value, data.data(), data.length());
}

JdwpValue::JdwpValue(IJdwpCon& con, JdwpTag t, const string& encoded) :
    tag(t) {
  string data = encoded.substr(0, GetSizeByTag(t, con));
  reverse(data.begin(), data.end());
  memcpy(&this->value, data.data(), data.length());
}

string JdwpValue::Serialize(IJdwpCon& con) const {
  return static_cast<char>(tag) + this->SerializeAsUntagged(con);
}

string JdwpValue::SerializeAsUntagged(IJdwpCon& con) const {
  string val = string(reinterpret_cast<const char*>(&this->value),
      sizeof(this->value));
  val = val.substr(0, GetSizeByTag(this->tag, con)); 
  reverse(val.begin(), val.end());

  return val;
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

JdwpArrayRegion::JdwpArrayRegion(IJdwpCon& con, const string& encoded) : 
    tag(static_cast<JdwpTag>(encoded[0])),
    values(new vector<unique_ptr<JdwpValue>>()) {
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
      this->values->push_back(unique_ptr<JdwpValue>(
           new JdwpValue(con, encoded.substr(i, i + value_size))));
    }
  } else {
    for (size_t i = kHeaderOffset;
        i < kHeaderOffset + value_size * value_count;
        i += value_size) {
      this->values->push_back(unique_ptr<JdwpValue>(
            new JdwpValue(con, this->tag, encoded.substr(i, value_size))));
    }
  }
}

string JdwpArrayRegion::Serialize(IJdwpCon& con) const {
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

