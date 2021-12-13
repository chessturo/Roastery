/* Specifies C++ representations of JDWP types
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

#include <cstdint>
#include <memory>
#include <vector>

#include "jdwp_con.hpp"

#ifndef ROASTERY_JDWP_TYPE_H_
#define ROASTERY_JDWP_TYPE_H_

namespace roastery {

/**
 * This namespace provides constants related to JDWP command/command set magic
 * numbers.
 */
namespace commands {

/**
 * Represents a JDWP command set.
 */
enum class CommandSet : uint8_t {
  kVirtualMachine = 1,
  kReferenceType = 2,
  kClassType = 3,
  kArrayType = 4,
  kInterfaceType = 5,
  kMethod = 6,
  kField = 8,
  kObjectReference = 9,
  kStringReference = 10,
  kThreadReference = 11,
  kThreadGroupReference = 12,
  kArrayReference = 13,
  kClassLoaderReference = 14,
  kEventRequest = 15,
  kStackFrame = 16,
  kClassObjectReference = 17,
  kEvent = 64,
};

/**
 * Represents a command in the \c VirtualMachine command set.
 */
enum class VirtualMachine : uint8_t {
  kVersion = 1,
  kClassesBySignature = 2,
  kAllClasses = 3,
  kAllThreads = 4,
  kTopLevelThreadGroups = 5,
  kDispose = 6,
  kIDSizes = 7,
  kSuspend = 8,
  kResume = 9,
  kExit = 10,
  kCreateString = 11,
  kCapabilities = 12,
  kClassPaths = 13,
  kDisposeObjects = 14,
  kHoldEvents = 15,
  kReleaseEvents = 16,
  kCapabilitiesNew = 17,
  kRedefineClassese = 18,
  kSetDefaultStratum = 19,
  kAllClassesWithGeneric = 20,
  kInstanceCounts = 21,
};

/**
 * Represents a command in the \c ReferenceType command set.
 */
enum class ReferenceType : uint8_t {
  kSignature = 1,
  kClassLoader = 2,
  kModifiers = 3,
  kFields = 4,
  kMethods = 5,
  kGetValues = 6,
  kSourceFile = 7,
  kNestedTypes = 8,
  kStatus = 9,
  kInterfaces = 10,
  kClassObject = 11,
  kSourceDebugExtension = 12,
  kSignatureWithGeneric = 13,
  kFieldsWithGeneric = 14,
  kMethodsWithGeneric = 15,
  kInstances = 16,
  kClassFileVersion = 17,
  kConstantPool = 18,
};

/**
 * Represents a command in the \c ClassType command set.
 */
enum class ClassType : uint8_t {
  kSuperclass = 1,
  kSetValues = 2,
  kInvokeMethod = 3,
  kNewInstance = 4,
};

/**
 * Represents a command in the \c ArrayType command set.
 */
enum class ArrayType : uint8_t {
  kNewInstance = 1,
};

/**
 * Represents a command in the \c Method command set.
 */
enum class Method : uint8_t {
  kLineTable = 1,
  kVariableTable = 2,
  kBytecodes = 3,
  kIsObsolete = 4,
  kVariableTableWithGeneric = 5,
};

/**
 * Represents a command in the \c ObjectReference command set.
 */
enum class ObjectReference : uint8_t {
  kReferenceType = 1,
  kGetValues = 2,
  kSetValues = 3,
  kMonitorInfo = 5,
  kInvokeMethod = 6,
  kDisableCollection = 7,
  kEnableCollection = 8,
  kIsCollected = 9,
  kReferringObjects = 10,
};

/**
 * Represents a command in the \c StringReference command set.
 */
enum class StringReference : uint8_t {
  kValue = 1,
};

/**
 * Represents a command in the \c ThreadReference command set.
 */
enum class ThreadReference : uint8_t {
  kName = 1,
  kSuspend = 2,
  kResume = 3,
  kStatus = 4,
  kThreadGroup = 5,
  kFrames = 6,
  kFrameCount = 7,
  kOwnedMonitors = 8,
  kCurrentContendedMonitor = 9,
  kStop = 10,
  kInterrupt = 11,
  kSuspendCount = 12,
  kOwnedMonitorsStackDepthInfo = 13,
  kForceEarlyReturn = 14,
};

/**
 * Represents a command in the \c ThreadGroupReference command set.
 */
enum class ThreadGroupReference : uint8_t {
  kName = 1,
  kParent = 2,
  kChildren = 3,
};

/**
 * Represents a command in the \c ArrayReference command set.
 */
enum class ArrayReference : uint8_t {
  kLength = 1,
  kGetValues = 2,
  kSetValues = 3,
};

/**
 * Represents a command in the \c ClassLoaderReference command set.
 */
enum class ClassLoaderReference : uint8_t {
  kVisibleClasses = 1,
};

/**
 * Represents a command in the \c EventRequest command set.
 */
enum class EventRequest : uint8_t {
  kSet = 1,
  kClear = 2,
  kClearAllBreakpoints = 3,
};

/**
 * Represents a command in the \c StackFrame command set.
 */
enum class StackFrame : uint8_t {
  kGetValues = 1,
  kSetValues = 2,
  kThisObject = 3,
  kPopFrames = 4,
};

/**
 * Represents a command in the \c ClassObjectReference command set.
 */
enum class ClassObjectReference : uint8_t {
  kReflectedType = 1,
};

/**
 * Represents a command in the \c Event command set.
 */
enum class Event : uint8_t {
  kComposite = 100,
};

}  // namespace commands

/**
 * An enum type for JDWP error codes. Note that these enum values are in
 * <em>host byte-order</em>, while JDWP error codes will be in <em>network
 * byte-order</em>.
 *
 * @see ntohs
 * @see htons
 */
enum class JdwpError : uint16_t {
  kNone = 0,

  kInvalidThread = 10,
  kInvalidThreadGroup = 11,
  kInvalidPriority = 12,
  kThreadNotSuspended = 13,
  kThreadSuspended = 14,
  kThreadNotAlive = 15,

  kInvalidObject = 20,
  kInvalidClass = 21,
  kClassNotPrepared = 22,
  kInvalidMethodId = 23,
  kInvalidLocation = 24,
  kInvalidFieldId = 25,

  kInvalidFrameId = 30,
  kNoMoreFrames = 31,
  kOpaqueFrame = 32,
  kNotCurrentFrame = 33,
  kTypeMismatch = 34,
  kInvalidSlot = 35,

  kDuplicate = 40,
  kNotFound = 41,

  kInvalidMonitor = 50,
  kNotMonitorOwner = 51,
  kInterrupt = 52,

  kInvalidClassFormat = 60,
  kCircularClassDefinition = 61,
  kFailsVerification = 62,
  kAddMethodNotImplemented = 63,
  kSchemaChangeNotImplemented = 64,
  kInvalidTypestate = 65,
  kHierarchyChangeNotImplemented = 66,
  kDeleteMethodNotImplemented = 67,
  kUnsupportedVersion = 68,
  kNamesDontMatch = 69,
  kClassModifiersChangeNotImplemented = 70,
  kMethodModifiersChangeNotImplemented = 71,

  kNotImplemented = 99,
  kNullPointer = 100,
  kAbsentInformation = 101,
  kInvalidEventType = 102,
  kIllegalArgument = 103,

  kOutOfMemory = 110,
  kAccessDenied = 111,
  kVmDead = 112,
  kInternal = 113,
  kUnattachedThread = 115,

  kInvalidTag = 500,
  kAlreadyInvoking = 502,
  kInvalidIndex = 503,
  kInvalidLength = 504,
  kInvalidString = 506,
  kInvalidClassLoader = 507,
  kInvalidArray = 508,
  kTransportLoad = 509,
  kTransportInit = 510,
  kNativeMethod = 511,
  kInvalidCount = 512,
};

/**
 * Returns the description for the given \c JdwpError listed in the spec.
 *
 * @param e The error code to query.
 *
 * @return A string description of the error.
 */
const string& jdwp_strerror(JdwpError e);

using std::string;
using std::unique_ptr;
using std::vector;

enum class JdwpTag : uint8_t {
  kArray = '[',
  kByte = 'B',
  kChar = 'C',
  kObject = 'L',
  kFloat = 'F',
  kDouble = 'D',
  kInt = 'I',
  kLong = 'J',
  kShort = 'S',
  kVoid = 'V',
  kBoolean = 'Z',
  kString = 's',
  kThread = 't',
  kThreadGroup = 'g',
  kClassLoader = 'l',
  kClassObject = 'c',
};

enum class JdwpTypeTag : uint8_t {
  kClass = 1,
  kInterface = 2,
  kArray = 3,
};

typedef int8_t JdwpByte;
/** 0 for false, non-zero for true */
typedef int8_t JdwpBool;
typedef uint16_t JdwpChar;
typedef uint32_t JdwpFloat;
typedef uint64_t JdwpDouble;
typedef int32_t JdwpInt;
typedef int64_t JdwpLong;
typedef int16_t JdwpShort;

typedef uint64_t JdwpObjId;
typedef JdwpObjId JdwpThreadId;
typedef JdwpObjId JdwpThreadGroupId;
typedef JdwpObjId JdwpStringId;
typedef JdwpObjId JdwpClassLoaderId;
typedef JdwpObjId JdwpClassObjectId;
typedef JdwpObjId JdwpArrayId;
typedef JdwpObjId ReferenceTypeId;
typedef ReferenceTypeId JdwpClassId;
typedef ReferenceTypeId JdwpInterfaceId;
typedef ReferenceTypeId JdwpArrayTypeId;

typedef int64_t JdwpMethodId;
typedef int64_t JdwpFieldId;
typedef int64_t JdwpFrameId;

/**
 * This type is a union of the JDWP primative types and \c JdwpObjId.
 */
typedef union {
  JdwpObjId obj;
  JdwpByte jbyte;
  JdwpBool jbool;
  JdwpShort jshort;
  JdwpChar jchar;
  JdwpInt jint;
  JdwpFloat jfloat;
  JdwpDouble jdouble;
  JdwpLong jlong;
} JdwpValUnion;

/**
 * This struct represents a tagged JDWP object ID.
 */
struct JdwpTaggedObjectId {
  /**
   * A tag that gives the type of \c this->obj_id.
   */
  JdwpTag tag;
  /**
   * The underlying object id.
   */
  JdwpObjId obj_id;

  /**
   * Constructs a \c JdwpTaggedObjectId with uninitialized values.
   */
  explicit JdwpTaggedObjectId();
  /** 
   * Constructs a \c JdwpTaggedObjectId based on the given JDWP encoded
   * string.
   *
   * @param data The data to parse.
   * @param con The connection \c data was received on; used to get needed
   * deserialization info.
   */
  explicit JdwpTaggedObjectId(const string& data, IJdwpCon& con);
  /**
   * Constructs a \c JdwpTaggedObjectId based on the tag and object id given.
   * This constructor exists only as a convience for setting multiple fields
   * simultaneously.
   */
  explicit JdwpTaggedObjectId(JdwpTag tag, JdwpObjId obj_id);
  /**
   * Returns a version of this \c TaggedObjectId encoded for JDWP.
   *
   * @param con The connection being written for. 
   */
  string Serialize(IJdwpCon& con) const;
};

/**
 * Represents a JDWP Location field.
 */
struct JdwpLocation {
  /**
   * Identifies whether this location is in a class or an interface.
   */
  JdwpTypeTag type;
  /**
   * Identifies which class/interface the location is in.
   */
  JdwpClassId class_id;
  /**
   * Identifies which method the location is in.
   */
  JdwpMethodId method_id;
  /**
   * The index of the location within the method.
   *
   * There are a few rules about how these are laid out:
   * <ul>
   * <li>The index of the start location for the method is less than all other
   * locations in the method.</li>
   * <li>The index of the end location for the method is greater than all other
   * locations in the method.</li>
   * <li>If a line number table exists for a method, locations that belong to a
   * particular line must fall between the line's location index and the
   * location index of the next line in the table
   * </ul>
   */
  uint64_t index;

  /**
   * Constructs a \c JdwpLocation with uninitialized values.
   */
  JdwpLocation();
  /**
   * Constructs a \c JdwpLocation with the specified parameter. This constructor
   * exists only as a convience for setting multiple fields simultaneously.
   */
  explicit JdwpLocation(JdwpTypeTag type, JdwpClassId class_id,
      JdwpMethodId method_id, uint64_t index);
  /**
   * Constructs a JdwpLocation from a JDWP encoded version.
   *
   * @param encoded The data to decode
   * @param con The connection the data was sent over. Used to get appropriate
   * deserialization information.
   */
  explicit JdwpLocation(const string& encoded, IJdwpCon& con);

  /**
   * Returns a \c string version of this \c JdwpLocation encoded for JDWP.
   * 
   * @param con The connection the data will be sent over. Used to get
   * appropriate serialization information.
   */
  string Serialize(IJdwpCon& con) const;
};

/**
 * This struct represents a JDWP String \em value.
 */
struct JdwpString {
  public:
    string data;

    /**
     * Constructs an empty \c JdwpString.
     */
    JdwpString();

    /**
     * Creates a <code>std::unique_ptr&lt;JdwpString&gt;</code> from JDWP
     * encoded data.
     */
    static JdwpString fromSerialized(const string& data);
    /**
     * Creates a <code>std::unique_ptr&lt;JdwpString&gt;</code> from host byte
     * order data.
     */
    static JdwpString fromHost(const string& data);

    /**
     * Returns a \c string version of this \c JdwpString encoded for JDWP.
     */
    string Serialize() const;
  private:
    JdwpString(const string& data);
};

/**
 * This struct represents a tagged or untagged JDWP value.
 */
struct JdwpValue {
  /**
   * The type of the underlying value. Any type that maps to some subset of
   * \c JdwpObjId maps to a \c JdwpObjId in the \c value union.
   */
  JdwpTag tag;
  /**
   * The underlying value of this \c JdwpValue. The type is given by
   * \c this->tag. Any type that is a subset of \c JdwpObjId is stored in the
   * union as a \c JdwpObjId.
   */
  JdwpValUnion value;

  /**
   * Constructs a \c JdwpValue with uninitialized values.
   */
  JdwpValue();
  /**
   * Constructs a \c JdwpValue with the specified parameter. This constructor
   * exists only as a convience for setting multiple fields simultaneously.
   */
  JdwpValue(JdwpTag tag, JdwpValUnion val);
  /**
   * Constructs a \c JdwpValue from the JDWP encoded version. Use the overload
   * <code>JdwpValue(JdwpTag, const string&)</code> if \c encoded is an
   * untagged value
   *
   * @param con The connection \c encoded was read from. Used to get needed
   * deserialization info.
   * @param encoded The encoded data.
   */
  JdwpValue(IJdwpCon& con, const string& encoded);
  /**
   * Constructs a \c JdwpValue from a JDWP untagged value.
   *
   * @param con The connection \c encoded was read from. Used to get needed
   * deserialization info.
   * @param t The type of the encoded value.
   * @param encoded The encoded data.
   */
  JdwpValue(IJdwpCon& con, JdwpTag t, const string& encoded);

  /**
   * Returns a \c string version of this \c JdwpValue encoded for JDWP.
   *
   * @param con The connection the serialized value will be sent over. Used to
   * get needed serialization info.
   */
  string Serialize(IJdwpCon& con) const;
  /**
   * Returns a \c string version of this \c JdwpValue as a JDWP untagged value.
   *
   * @param con The connection the serialized value will be sent over. Used to
   * get needed serialization info.
   */
  string SerializeAsUntagged(IJdwpCon& con) const;
};

/**
 * This struct represents a JDWP array region.
 */
struct JdwpArrayRegion {
  /**
   * Holds the tag representing the type of values in this array region.
   */
  JdwpTag tag;
  /**
   * The underlying values in this array region.
   */
  unique_ptr<vector<unique_ptr<JdwpValue>>> values;

  /**
   * Constructs a \c JdwpArrayRegion with uninitialized values.
   */
  JdwpArrayRegion();
  /**
   * Constructs a \c JdwpArrayRegion with the specified parameter. This
   * constructor exists only as a convience for setting multiple fields
   * simultaneously.
   */
  JdwpArrayRegion(JdwpTag tag, const vector<unique_ptr<JdwpValue>>& values);
  /**
   * Constructs a \c JdwpArrayRegion based off the JDWP encoded representation.
   *
   * @param con The connection \c encoded was recieved over.
   * @param encoded The JDWP encoded data.
   */
  JdwpArrayRegion(IJdwpCon& con, const string& encoded);

  /**
   * Returns a \c string version of this \c JdwpArrayRegion encoded for JDWP.
   */
  string Serialize(IJdwpCon& con) const;
};

}  // namespace roastery

#endif  // ROASTERY_JDWP_TYPE_H_

