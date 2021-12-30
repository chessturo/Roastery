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

#include <algorithm>
#include <cstdint>
#include <memory>
#include <sstream>
#include <variant>
#include <vector>

#include "jdwp_con.hpp"
#include "jdwp_exception.hpp"

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

enum class JdwpEventKind : uint8_t {
  kSingleStep = 1,
  kBreakpoint = 2,
  kFramePop = 3,
  kException = 4,
  kUserDefined = 5,
  kThreadStart = 6,
  kThreadDeath = 7,
  kClassPrepare = 8,
  kClassUnload = 9,
  kClassLoad = 10,
  kFieldAccess = 20,
  kFieldModification = 21,
  kExceptionCatch = 30,
  kMethodEntry = 40,
  kMethodExit = 41,
  kMethodExitWithReturnValue = 42,
  kMonitorContendedEnter = 43,
  kMonitorContendedEntered = 44,
  kMonitorWait = 45,
  kMonitorWaited = 46,
  kVmStart = 90,
  kVmDeath = 99,
};

/**
 * An interface for holding JDWP fields.
 */
struct IJdwpField {
  public:
    /**
     * Populates the data of \c this with the data encoded in \c encoded.
     *
     * @return The number of bytes read from \c encoded.
     */
    size_t FromEncoded(const string& encoded, IJdwpCon& con);
    /**
     * Serializes \c this.
     */
    string Serialize(IJdwpCon& con) const;
    virtual ~IJdwpField() = 0;
  protected:
    /**
     * Provides the implementation for \c Serialize.
     */
    virtual string SerializeImpl(IJdwpCon& con) const = 0;
    virtual size_t FromEncodedImpl(const string& encoded, IJdwpCon& con) = 0;
};

namespace impl {

template<typename Derived, typename UnderlyingType>
class JdwpFieldBase : public IJdwpField {
  public:
    static constexpr size_t value_size = sizeof(UnderlyingType);

    /**
     * Sets the value of \c this to \c v
     */
    Derived& operator<<(UnderlyingType v) {
      this->value = v;
      return dynamic_cast<Derived&>(*this);
    }

    /**
     * Returns the underlying value
     */
    UnderlyingType GetValue() { return this->value; }

    virtual ~JdwpFieldBase() = 0;
  protected:
    /**
     * Returns a serialization of \c value. Assumes \c value is numeric and
     * therefore does a byte-order conversion. This method can be overriden to
     * provide custom behavior.
     */
    virtual string SerializeImpl(IJdwpCon& con) const override {
      // Ignore con, we're assuming a numeric type which is of a fixed width.
      static_cast<void>(con);

      std::ostringstream res;
      const unsigned char *val_bytes =
        reinterpret_cast<const unsigned char*>(&value);

#ifdef __BIG_ENDIAN__
      for (const unsigned char* i = val_bytes;
          i < val_bytes + value_size;
          i++) {
        res << *i;
      }
#else
      // Loop backwards through the bytes of value to change byte order.
      for (const unsigned char* i = val_bytes + value_size - 1;
          i >= val_bytes;
          i--) {
        res << *i;
      }
#endif

      return res.str();
    }

    /**
     * Attemps to read a numeric value from encoded into \c value.
     */
    virtual size_t FromEncodedImpl(const string& encoded, IJdwpCon& con)
        override {
      static_cast<void>(con); // Assuming numeric type, which is fixed-width

      unsigned char* value_bytes = reinterpret_cast<unsigned char*>(&value);
#ifdef __BIG_ENDIAN__
      for (size_t i = 0; i < value_size; i++) {
        value_byes[i] = encoded[i];
      }
#else
      for (size_t i = 0; i < value_size; i++) {
        value_bytes[i] = encoded[value_size - 1 - i];
      }
#endif
      return value_size;
    }

    UnderlyingType value;
};

// Pure virtual dtor housekeeping
template<typename Derived, typename UnderlyingType>
JdwpFieldBase<Derived, UnderlyingType>::~JdwpFieldBase() { }

template <typename Derived, typename UnderlyingType>
class JdwpVariableSizeFieldBase :
    public JdwpFieldBase<Derived, UnderlyingType> {
  protected:
    size_t FromEncodedImpl(const string& encoded, IJdwpCon& con) override {
      uint8_t bytes_to_read = this->GetSize(con);
      vector<unsigned char> bytes;
      for (int i = 0; i < bytes_to_read; i++) {
        bytes.push_back(encoded[i]);
      }
      std::reverse(bytes.begin(), bytes.end());
      unsigned char* value_bytes =
        reinterpret_cast<unsigned char*>(&this->value);
      for (size_t i = 0; i < bytes.size(); i++) {
        value_bytes[i] = bytes[i];
      }

      return bytes_to_read;
    }

    string SerializeImpl(IJdwpCon& con) const override {
      uint8_t bytes_to_send = this->GetSize(con);
      if (bytes_to_send > sizeof(UnderlyingType)) {
        throw JdwpException("ID size too large");
      }
      string res;
#if __BIG_ENDIAN__
      res.append(reinterpret_cast<const char*>(&this->value), bytes_to_send);
#else
      const char* value_bytes =
        reinterpret_cast<const char*>(&this->value);
      for (const char* i = value_bytes + bytes_to_send - 1;
          i >= value_bytes;
          i--) {
        res.append(i, 1);
      }
#endif

      return res;
    }

    /**
     * Returns the size of this field on the given connection.
     */
    virtual uint8_t GetSize(IJdwpCon& con) const = 0;
};

}  // namespace impl

struct JdwpByte : impl::JdwpFieldBase<JdwpByte, uint8_t> { };
/** 0 for false, non-zero for true */
struct JdwpBool : impl::JdwpFieldBase<JdwpBool, uint8_t> { };
struct JdwpChar : impl::JdwpFieldBase<JdwpChar, int16_t> { };
struct JdwpFloat : impl::JdwpFieldBase<JdwpFloat, uint32_t> { };
struct JdwpDouble : impl::JdwpFieldBase<JdwpDouble, uint64_t> { };
struct JdwpInt : impl::JdwpFieldBase<JdwpInt, int32_t> { };
struct JdwpLong : impl::JdwpFieldBase<JdwpLong, int64_t> { };
struct JdwpShort : impl::JdwpFieldBase<JdwpShort, int16_t> { };

struct JdwpObjId : impl::JdwpVariableSizeFieldBase<JdwpObjId, uint64_t> {
  protected:
    virtual uint8_t GetSize(IJdwpCon& con) const override {
      return con.GetObjIdSize();
    }
};
struct JdwpThreadId : impl::JdwpVariableSizeFieldBase<JdwpThreadId, uint64_t> {
  protected:
    virtual uint8_t GetSize(IJdwpCon& con) const override {
      return con.GetObjIdSize();
    }
};
struct JdwpThreadGroupId :
    impl::JdwpVariableSizeFieldBase<JdwpThreadGroupId, uint64_t> {
  protected:
    virtual uint8_t GetSize(IJdwpCon& con) const override {
      return con.GetObjIdSize();
    }
};
struct JdwpStringId :
    impl::JdwpVariableSizeFieldBase<JdwpStringId, uint64_t> {
  protected:
    virtual uint8_t GetSize(IJdwpCon& con) const override {
      return con.GetObjIdSize();
    }
};
struct JdwpClassLoaderId :
    impl::JdwpVariableSizeFieldBase<JdwpClassLoaderId, uint64_t> {
  protected:
    virtual uint8_t GetSize(IJdwpCon& con) const override {
      return con.GetObjIdSize();
    }
};
struct JdwpClassObjectId :
    impl::JdwpVariableSizeFieldBase<JdwpClassObjectId, uint64_t> {
  protected:
    virtual uint8_t GetSize(IJdwpCon& con) const override {
      return con.GetObjIdSize();
    }
};
struct JdwpArrayId :
    impl::JdwpVariableSizeFieldBase<JdwpArrayId, uint64_t> {
  protected:
    virtual uint8_t GetSize(IJdwpCon& con) const override {
      return con.GetObjIdSize();
    }
};
struct JdwpReferenceTypeId :
    impl::JdwpVariableSizeFieldBase<JdwpReferenceTypeId, uint64_t> {
  protected:
    virtual uint8_t GetSize(IJdwpCon& con) const override {
      return con.GetObjIdSize();
    }
};
struct JdwpClassId :
    impl::JdwpVariableSizeFieldBase<JdwpClassId, uint64_t> {
  protected:
    virtual uint8_t GetSize(IJdwpCon& con) const override {
      return con.GetObjIdSize();
    }
};
struct JdwpInterfaceId :
    impl::JdwpVariableSizeFieldBase<JdwpInterfaceId, uint64_t> {
  protected:
    virtual uint8_t GetSize(IJdwpCon& con) const override {
      return con.GetObjIdSize();
    }
};
struct JdwpArrayTypeId :
    impl::JdwpVariableSizeFieldBase<JdwpArrayTypeId, uint64_t> {
  protected:
    virtual uint8_t GetSize(IJdwpCon& con) const override {
      return con.GetObjIdSize();
    }
};

struct JdwpMethodId :
    impl::JdwpVariableSizeFieldBase<JdwpMethodId, uint64_t> {
  protected:
    virtual uint8_t GetSize(IJdwpCon& con) const override {
      return con.GetMethodIdSize();
    }
};
struct JdwpFieldId :
    impl::JdwpVariableSizeFieldBase<JdwpFieldId, uint64_t> {
  protected:
    virtual uint8_t GetSize(IJdwpCon& con) const override {
      return con.GetFieldIdSize();
    }
};
struct JdwpFrameId :
    impl::JdwpVariableSizeFieldBase<JdwpFieldId, uint64_t> {
  protected:
    virtual uint8_t GetSize(IJdwpCon& con) const override {
      return con.GetFrameIdSize();
    }
};

/**
 * This struct represents a tagged JDWP object ID.
 */
struct JdwpTaggedObjectId : public IJdwpField {
  /**
   * A tag that gives the type of \c this->obj_id.
   */
  JdwpTag tag;
  /**
   * The underlying object id.
   */
  JdwpObjId obj_id;

  /**
   * Constructs an uninitialized \c JdwpTaggedObjectId.
   */
  JdwpTaggedObjectId();
  /**
   * Constructs a \c JdwpTaggedObjectId based on the tag and object id given.
   * This constructor exists only as a convience for setting multiple fields
   * simultaneously.
   */
  explicit JdwpTaggedObjectId(JdwpTag tag, JdwpObjId obj_id);
  protected:
    virtual size_t FromEncodedImpl(const string &encoded,
        IJdwpCon &con) override;
    virtual string SerializeImpl(IJdwpCon &con) const override;
};

/**
 * Represents a JDWP Location field.
 */
struct JdwpLocation : public IJdwpField {
  public:
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
     * There are a few rules about how these are laid out: <ul>
     *
     * <li>The index of the start location for the method is less than all
     * other locations in the method.</li>
     *
     * <li>The index of the end location for the method is greater than all
     * other locations in the method.</li>
     *
     * <li>If a line number table exists for a method, locations that belong to
     * a particular line must fall between the line's location index and the
     * location index of the next line in the table
     *
     * </ul>
     */
    uint64_t index;

    /**
     * Constructs an uninitialized \c JdwpLocation.
     */
    JdwpLocation();
    /**
     * Constructs a \c JdwpLocation with the specified parameter. This
     * constructor exists only as a convience for setting multiple fields
     * simultaneously.
     */
    explicit JdwpLocation(JdwpTypeTag type, JdwpClassId class_id,
        JdwpMethodId method_id, uint64_t index);

  protected:
    /**
     * Returns a \c string version of this \c JdwpLocation encoded for JDWP.
     *
     * @param con The connection the data will be sent over. Used to get
     * appropriate serialization information.
     */
    virtual string SerializeImpl(IJdwpCon& con) const override;
    virtual size_t FromEncodedImpl(const string& encoded,
        IJdwpCon& con) override;
};

/**
 * This struct represents a JDWP String \em value.
 */
struct JdwpString : IJdwpField {
  public:
    /**
     * Constructs an empty \c JdwpString.
     */
    JdwpString();
    JdwpString& operator<<(const string& s);
    string& GetValue();
    const string& GetValue() const;
  protected:
    virtual size_t FromEncodedImpl(const string &encoded,
        IJdwpCon& con) override;
    virtual string SerializeImpl(IJdwpCon& con) const override;
  private:
    string data;
    JdwpString(const string& data);
};

/**
 * This struct represents a tagged or untagged JDWP value.
 */
struct JdwpValue : IJdwpField {
  public:
    /**
     * Represents a JdwpValue with a type of void
     */
    struct VoidValue {
      static constexpr size_t value_size = 0;
      size_t FromEncoded(const string& encoded, IJdwpCon& con) {
        static_cast<void>(encoded);
        static_cast<void>(con);
        throw std::logic_error("Trying to read a void value");
      }
      string Serialize(IJdwpCon& con) const {
        static_cast<void>(con);
        return "";
      }
    };
    using JdwpVal = std::variant<
      VoidValue,
      JdwpBool,
      JdwpByte,
      JdwpChar,
      JdwpFloat,
      JdwpDouble,
      JdwpInt,
      JdwpLong,
      JdwpShort,
      JdwpObjId
      >;
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
    JdwpVal value;

    /**
     * Constructs a \c JdwpValue with uninitialized values.
     */
    JdwpValue();
    /**
     * Constructs a \c JdwpValue with the specified parameter. This constructor
     * exists only as a convience for setting multiple fields simultaneously.
     */
    JdwpValue(JdwpTag tag, JdwpVal val);
    /**
     * Constructs a \c JdwpValue from a JDWP untagged value.
     *
     * @param con The connection \c encoded was read from. Used to get needed
     * deserialization info.
     * @param t The type of the encoded value.
     * @param encoded The encoded data.
     */
    size_t FromEncodedAsUntagged(JdwpTag t, const string& encoded,
        IJdwpCon& con);

    /**
     * Returns a \c string version of this \c JdwpValue as a JDWP untagged
     * value.
     *
     * @param con The connection the serialized value will be sent over. Used to
     * get needed serialization info.
     */
    string SerializeAsUntagged(IJdwpCon& con) const;
  protected:
    string SerializeImpl(IJdwpCon& con) const override;
    size_t FromEncodedImpl(const string& encoded, IJdwpCon& con) override;
};

/**
 * This struct represents a JDWP array region.
 */
struct JdwpArrayRegion : IJdwpField {
  public:
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
  protected:
    string SerializeImpl(IJdwpCon& con) const override;
    size_t FromEncodedImpl(const string& encoded, IJdwpCon& con) override;
};

}  // namespace roastery

#endif  // ROASTERY_JDWP_TYPE_H_

