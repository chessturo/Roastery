/* Specifies an abstraction for Java Debug Wire Protocol messaging
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

#ifndef ROASTERY_JDWP_PACKET_H_
#define ROASTERY_JDWP_PACKET_H_

#include <arpa/inet.h>

#include <cstdint>
#include <sstream>
#include <vector>

#include "jdwp_con.hpp"
#include "jdwp_exception.hpp"
#include "jdwp_type.hpp"

namespace roastery {

using std::string;

/**
 * Holds flag values for JDWP. */
enum class JdwpFlags : uint8_t {
  kNone = 0x00,
  kReply = 0x80,
};

/**
 * Holds a JDWP command packet. May be a command packet that originates from the
 * debugger, or one that originates from the JVM (an event packet).
 */
class IJdwpCommandPacket {
  public:
    IJdwpCommandPacket();
    /**
     * Returns \c this serialized for transmission over JDWP.
     */
    string Serialize(IJdwpCon& con) const;
    virtual ~IJdwpCommandPacket() = 0;

    uint32_t GetId() const;
  protected:
    uint32_t id;
    /**
     * Provides an implementation of \c Serialize().
     */
    virtual string SerializeImpl(IJdwpCon& con) const = 0;
    /**
     * Generates a JDWP header for this command.
     *
     * @param body_len The length of the body/data in this JDWP packet. Should
     * not include the length of a JDWP command header. Should be in host
     * byte-order.
     * @param id The ID to be used in the header. Should be in host byte-order.
     */
    static string ProduceHeader(uint8_t command_set, uint8_t command,
        size_t body_len, uint32_t id);
};

// Template metaprogramming helpers
namespace impl {

using std::forward;
using std::get;
using std::tuple_size;

template <size_t rem>
class TupleForEachHelper {
  public:
    template <typename Tuple, typename Func>
    static void Exec(Tuple&& tuple, Func&& func) {
      constexpr size_t tuple_siz = tuple_size<std::decay_t<Tuple>>::value;
      static_assert(rem <= tuple_siz, "Bad size in TupleForEachHelper");
      constexpr size_t idx = tuple_siz - rem;
      func(get<idx>(tuple));
      TupleForEachHelper<rem - 1>
        ::Exec(forward<Tuple>(tuple), forward<Func>(func));
    }
};

// template recursion base case: 0 elements remaining
template <>
class TupleForEachHelper<0> {
  public:
    template <typename Tuple, typename Func>
    static void Exec(Tuple&& tuple, Func&& func) {
      static_cast<void>(tuple);
      static_cast<void>(func);
    }
};

/**
 * Runs a functor on each value in a \c std::tuple.
 *
 * @param tuple The tuple to iterate over
 * @param func The functor to execute on each value in \c tuple.
 */
template <typename Tuple, typename Func>
void TupleForEach(Tuple&& tuple, Func&& func) {
  TupleForEachHelper
    <tuple_size<std::decay_t<Tuple>>::value>
    ::Exec(forward<Tuple>(tuple), forward<Func>(func));
}

/**
 * Helper to determine if \c T is a \c std::vector.
 */
template<typename T>
class IsVector {
  public:
    static constexpr bool value = false;
};

template<typename T>
class IsVector<vector<T>> {
  public:
    static constexpr bool value = true;
};

/**
 * Returns the next free ID for an outgoing packet.
 */
uint32_t GetNextId();

const uint32_t kHeaderLen = 11;
/**
 * Provides a base for types representing JDWP command packets.
 *
 * @tparam command_set The JDWP command set of the command represented by this
 * packet
 * @tparam command The JDWP command ID represented by this packet
 * @tparam Fields a std::tuple of \c IJdwpField interspersed with
 * \c std::vectors of \c std::tuple of \c IJdwpField / \c std::vector
 * (recursively) combinations.
 */
template <uint8_t command_set, uint8_t command, typename Fields>
class CommandPacketBase : public roastery::IJdwpCommandPacket {
  public:
    CommandPacketBase() : IJdwpCommandPacket() { }

    Fields& GetFields() { return fields; }
    const Fields& GetFields() const { return fields; }

    virtual ~CommandPacketBase() = 0;
  protected:
    /**
     * Provides an implementation of \c Serialize(). Can be overriden by derived
     * classes for customization. By default, calls the \c Serialize() method
     * of each item in \c Fields and concatenates them. If any field is of type
     * \c std::vector, first conctenates a \c JdwpInt for the length of the
     * vector, and then each vector element.
     */
    virtual string SerializeImpl(IJdwpCon& con) const {
      std::stringstream body_acc;
      TupleForEach(this->fields, [&body_acc, &con](const auto& s) {
        RecursiveSerialize(body_acc, s, con);
      });
      string body = body_acc.str();
      return ProduceHeader(command_set, command, body.length(), this->id) +
        body;
    }
  private:
    /**
     * Recursively serializes \c field. If it's a vector type by prepending a
     * \c JdwpInt length and then recursively serializing each element.
     * Otherwise, just serializes the element.
     */
    template <typename Field>
    static void RecursiveSerialize(std::stringstream& acc, const Field& field,
        IJdwpCon& con) {
      if (IsVector<Field>::value) {
        RecursiveSerialize(acc, field, con);
      } else {
        acc << field.Serialize(con);
      }
    }

    template<typename FieldBundle>
    static void RecursiveSerialize(std::stringstream& acc,
        const vector<FieldBundle>& v, IJdwpCon& con) {
      JdwpInt len;
      len << v.size();
      acc << len.Serialize(con);
      for (const FieldBundle& bundle : v) {
        TupleForEach(bundle, [&acc, &con](const auto& s) {
          RecursiveSerialize(acc, s, con);
        });
      }
    }

    Fields fields;
};

template <uint8_t command_set, uint8_t command, typename Fields>
CommandPacketBase<command_set, command, Fields>::~CommandPacketBase() { }

}  // namespace impl

namespace command_packets {

using namespace commands;

using std::tuple;
using impl::CommandPacketBase;

namespace virtual_machine {

constexpr uint8_t kVm = static_cast<uint8_t>(CommandSet::kVirtualMachine);

// Virtual Machine command set
class VersionCommand :
    public CommandPacketBase<
      kVm,
      static_cast<uint8_t>(VirtualMachine::kVersion),
      tuple<>
    > { };

class ClassesBySignatureCommand :
    public CommandPacketBase<
      kVm,
      static_cast<uint8_t>(VirtualMachine::kClassesBySignature),
      tuple<JdwpString>
    > { };

class AllClassesCommand :
    public CommandPacketBase<
      kVm,
      static_cast<uint8_t>(VirtualMachine::kAllClasses),
      tuple<>
    > { };

class AllThreadsCommand :
    public CommandPacketBase<
      kVm,
      static_cast<uint8_t>(VirtualMachine::kAllThreads),
      tuple<>
    > { };

class TopLevelThreadGroupsCommand :
    public CommandPacketBase<
      kVm,
      static_cast<uint8_t>(VirtualMachine::kTopLevelThreadGroups),
      tuple<>
    > { };

class DisposeCommand :
    public CommandPacketBase<
      kVm,
      static_cast<uint8_t>(VirtualMachine::kDispose),
      tuple<>
    > { };

class IDSizesCommand :
    public CommandPacketBase<
      kVm,
      static_cast<uint8_t>(VirtualMachine::kIDSizes),
      tuple<>
    > { };

class SuspendCommand :
    public CommandPacketBase<
      kVm,
      static_cast<uint8_t>(VirtualMachine::kSuspend),
      tuple<>
    > { };

class ResumeCommand :
    public CommandPacketBase<
      kVm,
      static_cast<uint8_t>(VirtualMachine::kResume),
      tuple<>
    > { };

class ExitCommand :
    public CommandPacketBase<
      kVm,
      static_cast<uint8_t>(VirtualMachine::kExit),
      tuple<JdwpInt>
    > { };

class CreateStringCommand :
    public CommandPacketBase<
      kVm,
      static_cast<uint8_t>(VirtualMachine::kCreateString),
      tuple<JdwpString>
    > { };

class CapabilitiesCommand :
    public CommandPacketBase<
      kVm,
      static_cast<uint8_t>(VirtualMachine::kCapabilities),
      tuple<>
    > { };

class ClassPathsCommand :
    public CommandPacketBase<
      kVm,
      static_cast<uint8_t>(VirtualMachine::kClassPaths),
      tuple<>
    > { };

class DisposeObjectsCommand :
    public CommandPacketBase<
      kVm,
      static_cast<uint8_t>(VirtualMachine::kDisposeObjects),
      tuple<vector<tuple<JdwpObjId, JdwpInt>>>
    > { };

class HoldEventsCommand :
    public CommandPacketBase<
      kVm,
      static_cast<uint8_t>(VirtualMachine::kHoldEvents),
      tuple<>
    > { };

class ReleaseEventsCommand :
    public CommandPacketBase<
      kVm,
      static_cast<uint8_t>(VirtualMachine::kReleaseEvents),
      tuple<>
    > { };

class CapabilitiesNewCommand :
    public CommandPacketBase<
      kVm,
      static_cast<uint8_t>(VirtualMachine::kCapabilitiesNew),
      tuple<>
    > { };

class RedefineClassesCommand :
    public CommandPacketBase<
      kVm,
      static_cast<uint8_t>(VirtualMachine::kRedefineClassese),
      tuple<vector<tuple<JdwpReferenceTypeId, vector<JdwpByte>>>>
    > { };

class SetDefaultStratumCommand :
    public CommandPacketBase<
      kVm,
      static_cast<uint8_t>(VirtualMachine::kSetDefaultStratum),
      tuple<JdwpString>
    > { };

class AllClassesWithGenericCommand :
    public CommandPacketBase<
      kVm,
      static_cast<uint8_t>(VirtualMachine::kAllClassesWithGeneric),
      tuple<>
    > { };

class InstanceCounts :
    public CommandPacketBase<
      kVm,
      static_cast<uint8_t>(VirtualMachine::kInstanceCounts),
      tuple<vector<tuple<JdwpReferenceTypeId>>>
    > { };

}  // namespace virtual_machine

namespace reference_type {

constexpr uint8_t kRefType = static_cast<uint8_t>(CommandSet::kReferenceType);

class SignatureCommand :
    public CommandPacketBase<
      kRefType,
      static_cast<uint8_t>(ReferenceType::kSignature),
      tuple<JdwpReferenceTypeId>
    > { };

class ClassLoaderCommand :
    public CommandPacketBase<
      kRefType,
      static_cast<uint8_t>(ReferenceType::kClassLoader),
      tuple<JdwpReferenceTypeId>
    > { };

class ModifiersCommand :
    public CommandPacketBase<
      kRefType,
      static_cast<uint8_t>(ReferenceType::kModifiers),
      tuple<JdwpReferenceTypeId>
    > { };

class FieldsCommand :
    public CommandPacketBase<
      kRefType,
      static_cast<uint8_t>(ReferenceType::kFields),
      tuple<JdwpReferenceTypeId>
    > { };

class MethodsCommand :
    public CommandPacketBase<
      kRefType,
      static_cast<uint8_t>(ReferenceType::kMethods),
      tuple<JdwpReferenceTypeId>
    > { };

class GetValuesCommand :
    public CommandPacketBase<
      kRefType,
      static_cast<uint8_t>(ReferenceType::kGetValues),
      tuple<JdwpReferenceTypeId, vector<tuple<JdwpFieldId>>>
    > { };

class SourceFileCommand :
    public CommandPacketBase<
      kRefType,
      static_cast<uint8_t>(ReferenceType::kSourceFile),
      tuple<JdwpReferenceTypeId>
    > { };

class NestedTypesCommand :
    public CommandPacketBase<
      kRefType,
      static_cast<uint8_t>(ReferenceType::kNestedTypes),
      tuple<JdwpReferenceTypeId>
    > { };

class StatusCommand :
    public CommandPacketBase<
      kRefType,
      static_cast<uint8_t>(ReferenceType::kStatus),
      tuple<JdwpReferenceTypeId>
    > { };

class InterfacesCommand :
    public CommandPacketBase<
      kRefType,
      static_cast<uint8_t>(ReferenceType::kInterfaces),
      tuple<JdwpReferenceTypeId>
    > { };

class ClassObjectCommand :
    public CommandPacketBase<
      kRefType,
      static_cast<uint8_t>(ReferenceType::kClassObject),
      tuple<JdwpReferenceTypeId>
    > { };

class SourceDebugExtensionCommand :
    public CommandPacketBase<
      kRefType,
      static_cast<uint8_t>(ReferenceType::kSourceDebugExtension),
      tuple<JdwpReferenceTypeId>
    > { };

class SignatureWithGenericCommand :
    public CommandPacketBase<
      kRefType,
      static_cast<uint8_t>(ReferenceType::kSignatureWithGeneric),
      tuple<JdwpReferenceTypeId>
    > { };

class FieldsWithGenericCommand :
    public CommandPacketBase<
      kRefType,
      static_cast<uint8_t>(ReferenceType::kFieldsWithGeneric),
      tuple<JdwpReferenceTypeId>
    > { };

class MethodsWithGenericCommand :
    public CommandPacketBase<
      kRefType,
      static_cast<uint8_t>(ReferenceType::kMethodsWithGeneric),
      tuple<JdwpReferenceTypeId>
    > { };

class InstancesCommand :
    public CommandPacketBase<
      kRefType,
      static_cast<uint8_t>(ReferenceType::kInstances),
      tuple<JdwpReferenceTypeId, JdwpInt>
    > { };

class ClassFileVersionCommand :
    public CommandPacketBase<
      kRefType,
      static_cast<uint8_t>(ReferenceType::kClassFileVersion),
      tuple<JdwpReferenceTypeId>
    > { };

class ConstantPoolCommand :
    public CommandPacketBase<
      kRefType,
      static_cast<uint8_t>(ReferenceType::kConstantPool),
      tuple<JdwpReferenceTypeId>
    > { };

}  // namespace reference_type

namespace class_type {

constexpr uint8_t kClassType = static_cast<uint8_t>(CommandSet::kClassType);

class SuperclassCommand :
    public CommandPacketBase<
      kClassType,
      static_cast<uint8_t>(ClassType::kSuperclass),
      tuple<JdwpClassId>
    > { };

class SetValuesCommand :
    public CommandPacketBase<
      kClassType,
      static_cast<uint8_t>(ClassType::kSetValues),
      tuple<JdwpClassId, vector<tuple<JdwpFieldId, JdwpValue>>>
    > {
  protected:
    // Because the JdwpValues need to be untagged, which isn't the default,
    // override
    string SerializeImpl(IJdwpCon& con) const override;
};

class InvokeMethodCommand :
    public CommandPacketBase<
      kClassType,
      static_cast<uint8_t>(ClassType::kInvokeMethod),
      tuple<JdwpClassId, JdwpThreadId, JdwpMethodId, vector<tuple<JdwpValue>>,
        JdwpInt>
    > { };

class NewInstanceCommand :
    public CommandPacketBase<
      kClassType,
      static_cast<uint8_t>(ClassType::kNewInstance),
      tuple<JdwpClassId, JdwpThreadId, JdwpMethodId, vector<tuple<JdwpValue>>,
        JdwpInt>
    > { };

}  // namespace class_type

namespace array_type {

class NewInstanceCommand :
    public CommandPacketBase<
      static_cast<uint8_t>(CommandSet::kArrayType),
      static_cast<uint8_t>(ArrayType::kNewInstance),
      tuple<JdwpArrayTypeId, JdwpInt>
    > { };

}  // namespace array_type

namespace interface_type {

// Currently no commands defined in the JDWP spec for the InterfaceType command
// set.

}  // namespace interface_type

namespace method {

constexpr uint8_t kMethod = static_cast<uint8_t>(CommandSet::kMethod);

class LineTableCommand :
    public CommandPacketBase<
      kMethod,
      static_cast<uint8_t>(Method::kLineTable),
      tuple<JdwpReferenceTypeId, JdwpMethodId>
    > { };

class VariableTableCommand :
    public CommandPacketBase<
      kMethod,
      static_cast<uint8_t>(Method::kVariableTable),
      tuple<JdwpReferenceTypeId, JdwpMethodId>
    > { };

class BytecodesCommand :
    public CommandPacketBase<
      kMethod,
      static_cast<uint8_t>(Method::kBytecodes),
      tuple<JdwpReferenceTypeId, JdwpMethodId>
    > { };

class IsObsoleteCommand :
    public CommandPacketBase<
      kMethod,
      static_cast<uint8_t>(Method::kIsObsolete),
      tuple<JdwpReferenceTypeId, JdwpMethodId>
    > { };

class VariableTableWithGenericCommand :
    public CommandPacketBase<
      kMethod,
      static_cast<uint8_t>(Method::kVariableTableWithGeneric),
      tuple<JdwpReferenceTypeId, JdwpMethodId>
    > { };

}  // namespace method

namespace field {

// The field command set currently has no associated commands

}  // namespace field

namespace object_reference {

constexpr uint8_t kObjRef = static_cast<uint8_t>(CommandSet::kObjectReference);

class ReferenceTypeCommand :
    CommandPacketBase<
      kObjRef,
      static_cast<uint8_t>(ObjectReference::kReferenceType),
      tuple<JdwpObjId>
    > { };

class GetValuesCommand :
    CommandPacketBase<
      kObjRef,
      static_cast<uint8_t>(ObjectReference::kGetValues),
      tuple<JdwpObjId, vector<tuple<JdwpFieldId>>>
    > { };

class SetValuesCommand :
    CommandPacketBase<
      kObjRef,
      static_cast<uint8_t>(ObjectReference::kSetValues),
      tuple<JdwpObjId, vector<tuple<JdwpFieldId, JdwpValue>>>
    > {
  protected:
    // Need to serialize the values as untagged, which isn't the default, so
    // override
    string SerializeImpl(IJdwpCon &con) const override;
};

class MonitorInfoCommand :
    CommandPacketBase<
      kObjRef,
      static_cast<uint8_t>(ObjectReference::kMonitorInfo),
      tuple<JdwpObjId>
    > { };

class InvokeMethodCommand :
    CommandPacketBase<
      kObjRef,
      static_cast<uint8_t>(ObjectReference::kInvokeMethod),
      tuple<JdwpObjId, JdwpThreadId, JdwpClassId, JdwpMethodId,
        vector<tuple<JdwpValue>>, JdwpInt>
    > { };

class DisableCollectionCommand :
    CommandPacketBase<
      kObjRef,
      static_cast<uint8_t>(ObjectReference::kDisableCollection),
      tuple<JdwpObjId>
    > { };

class EnableCollectionCommand :
    CommandPacketBase<
      kObjRef,
      static_cast<uint8_t>(ObjectReference::kEnableCollection),
      tuple<JdwpObjId>
    > { };

class IsCollectedCommand :
    CommandPacketBase<
      kObjRef,
      static_cast<uint8_t>(ObjectReference::kIsCollected),
      tuple<JdwpObjId>
    > { };

class ReferringObjectsCommand :
    CommandPacketBase<
      kObjRef,
      static_cast<uint8_t>(ObjectReference::kReferringObjects),
      tuple<JdwpObjId, JdwpInt>
    > { };

}  // namespace object_reference

namespace string_reference {

class ValueCommand :
    CommandPacketBase<
      static_cast<uint8_t>(CommandSet::kStringReference),
      static_cast<uint8_t>(StringReference::kValue),
      tuple<JdwpObjId>
    > { };

}  // namespace string_reference

namespace thread_reference {

constexpr uint8_t kThrdRef = static_cast<uint8_t>(CommandSet::kThreadReference);

class NameCommand :
    public CommandPacketBase<
      kThrdRef,
      static_cast<uint8_t>(ThreadReference::kName),
      tuple<JdwpThreadId>
    > { };

class SuspendCommand :
    public CommandPacketBase<
      kThrdRef,
      static_cast<uint8_t>(ThreadReference::kSuspend),
      tuple<JdwpThreadId>
    > { };

class ResumeCommand :
    public CommandPacketBase<
      kThrdRef,
      static_cast<uint8_t>(ThreadReference::kResume),
      tuple<JdwpThreadId>
    > { };

class StatusCommand :
    public CommandPacketBase<
      kThrdRef,
      static_cast<uint8_t>(ThreadReference::kStatus),
      tuple<JdwpThreadId>
    > { };

class ThreadGroupCommand :
    public CommandPacketBase<
      kThrdRef,
      static_cast<uint8_t>(ThreadReference::kThreadGroup),
      tuple<JdwpThreadId>
    > { };

class FramesCommand :
    public CommandPacketBase<
      kThrdRef,
      static_cast<uint8_t>(ThreadReference::kFrames),
      tuple<JdwpThreadId, JdwpInt, JdwpInt>
    > { };

class FrameCountCommand :
    public CommandPacketBase<
      kThrdRef,
      static_cast<uint8_t>(ThreadReference::kFrameCount),
      tuple<JdwpThreadId>
    > { };

class OwnedMonitorsCommand :
    public CommandPacketBase<
      kThrdRef,
      static_cast<uint8_t>(ThreadReference::kOwnedMonitors),
      tuple<JdwpThreadId>
    > { };

class CurrentContendedMonitorCommand :
    public CommandPacketBase<
      kThrdRef,
      static_cast<uint8_t>(ThreadReference::kCurrentContendedMonitor),
      tuple<JdwpThreadId>
    > { };

class StopCommand :
    public CommandPacketBase<
      kThrdRef,
      static_cast<uint8_t>(ThreadReference::kStop),
      tuple<JdwpThreadId, JdwpObjId>
    > { };

class InterruptCommand :
    public CommandPacketBase<
      kThrdRef,
      static_cast<uint8_t>(ThreadReference::kInterrupt),
      tuple<JdwpThreadId>
    > { };

class SuspendCountCommand :
    public CommandPacketBase<
      kThrdRef,
      static_cast<uint8_t>(ThreadReference::kSuspendCount),
      tuple<JdwpThreadId>
    > { };

class OwnedMonitorsStackDepthInfoCommand :
    public CommandPacketBase<
      kThrdRef,
      static_cast<uint8_t>(ThreadReference::kOwnedMonitorsStackDepthInfo),
      tuple<JdwpThreadId>
    > { };

class ForceEarlyReturnCommand :
    public CommandPacketBase<
      kThrdRef,
      static_cast<uint8_t>(ThreadReference::kForceEarlyReturn),
      tuple<JdwpThreadId, JdwpValue>
    > { };

}  // namespace thread_reference

namespace thread_group_reference {

constexpr uint8_t kThrdGrpRef =
  static_cast<uint8_t>(CommandSet::kThreadGroupReference);

class NameCommand :
    public CommandPacketBase<
      kThrdGrpRef,
      static_cast<uint8_t>(ThreadGroupReference::kName),
      tuple<JdwpThreadGroupId>
    > { };

class ParentCommand :
    public CommandPacketBase<
      kThrdGrpRef,
      static_cast<uint8_t>(ThreadGroupReference::kParent),
      tuple<JdwpThreadGroupId>
    > { };

class ChildrenCommand :
    public CommandPacketBase<
      kThrdGrpRef,
      static_cast<uint8_t>(ThreadGroupReference::kChildren),
      tuple<JdwpThreadGroupId>
    > { };

}  // namespace thread_group_reference

namespace array_reference {

constexpr uint8_t kArrRef = static_cast<uint8_t>(CommandSet::kArrayReference);

class LengthCommand :
    public CommandPacketBase<
      kArrRef,
      static_cast<uint8_t>(ArrayReference::kLength),
      tuple<JdwpArrayId>
    > { };

class GetValuesCommand :
    public CommandPacketBase<
      kArrRef,
      static_cast<uint8_t>(ArrayReference::kGetValues),
      tuple<JdwpArrayId, JdwpInt, JdwpInt>
    > { };

class SetValuesCommand :
    public CommandPacketBase<
      kArrRef,
      static_cast<uint8_t>(ArrayReference::kSetValues),
      tuple<JdwpArrayId, JdwpInt, vector<tuple<JdwpValue>>>
    > {
  protected:
    // Need overriden impl to serialize JdwpValues as untagged.
    string SerializeImpl(IJdwpCon& con) const override;
};

}  // namespace array_reference

namespace class_loader_reference {

class VisibleClassesCommand :
    public CommandPacketBase<
      static_cast<uint8_t>(CommandSet::kClassLoaderReference),
      static_cast<uint8_t>(ClassLoaderReference::kVisibleClasses),
      tuple<JdwpClassLoaderId>
    > { };

}  // namespace class_loader_reference

namespace event_request {

constexpr uint8_t kEvReq = static_cast<uint8_t>(CommandSet::kEventRequest);

// Fully custom implementation due to odd serialization based on modKind
class SetCommand : public IJdwpCommandPacket {
  public:
    // modKind is given by the index into the variant + 1 (since the indexes are
    // zero based, but modKind starts at 1).
    using Modifier = std::variant<
      tuple<JdwpInt>,
      tuple<JdwpInt>,
      tuple<JdwpThreadId>,
      tuple<JdwpReferenceTypeId>,
      tuple<JdwpString>,
      tuple<JdwpString>,
      tuple<JdwpLocation>,
      tuple<JdwpReferenceTypeId, JdwpBool, JdwpBool>,
      tuple<JdwpReferenceTypeId, JdwpFieldId>,
      tuple<JdwpThreadId, JdwpInt, JdwpInt>,
      tuple<JdwpObjId>,
      tuple<JdwpString>
      >;
    using Fields = tuple<JdwpByte, JdwpByte, vector<Modifier>>;

    SetCommand();

    Fields& GetFields();
    const Fields& GetFields() const;
  protected:
    string SerializeImpl(IJdwpCon& con) const override;
  private:
    Fields fields;
};

class ClearCommand :
    public CommandPacketBase<
      kEvReq,
      static_cast<uint8_t>(EventRequest::kClear),
      tuple<JdwpByte, JdwpInt>
    > { };

class ClearAllBreakpointsCommand :
    public CommandPacketBase<
      kEvReq,
      static_cast<uint8_t>(EventRequest::kClearAllBreakpoints),
      tuple<>
    > { };

}  // namespace event_request

namespace stack_frame {

constexpr uint8_t kStkFrm = static_cast<uint8_t>(CommandSet::kStackFrame);

class GetValuesCommand :
    public CommandPacketBase<
      kStkFrm,
      static_cast<uint8_t>(StackFrame::kGetValues),
      tuple<JdwpThreadId, JdwpFrameId, vector<tuple<JdwpInt, JdwpByte>>>
    > { };

class SetValuesCommand :
    public CommandPacketBase<
      kStkFrm,
      static_cast<uint8_t>(StackFrame::kSetValues),
      tuple<JdwpThreadId, JdwpFrameId, vector<tuple<JdwpInt, JdwpValue>>>
    > { };

class ThisObjectCommand :
    public CommandPacketBase<
      kStkFrm,
      static_cast<uint8_t>(StackFrame::kThisObject),
      tuple<JdwpThreadId, JdwpFrameId>
    > { };

class PopFramesCommand :
    public CommandPacketBase<
      kStkFrm,
      static_cast<uint8_t>(StackFrame::kPopFrames),
      tuple<JdwpThreadId, JdwpFrameId>
    > { };

}  // namespace stack_frame

namespace class_object_reference {

class ReflectedTypeCommand :
    public CommandPacketBase<
      static_cast<uint8_t>(CommandSet::kClassObjectReference),
      static_cast<uint8_t>(ClassObjectReference::kReflectedType),
      tuple<JdwpClassObjectId>
    > { };

}  // namespace class_object_reference

}  // namespace command_packets

/**
 * Returns \c true when the packet with the given \c header is an event packet,
 * \c false otherwise.
 */
bool HeaderIsEvent(const string& header);

class Handler;
/**
 * Represents a single event within a JDWP Composite Event.
 */
class IJdwpEvent {
  public:
    /**
     * Parses a JDWP composite event into a \c vector of the contained events.
     *
     * @param encoded The JDWP encoded composite event, including the JDWP
     * header.
     * @param con The JDWP connection \c encoded was recieved from.
     *
     * @return A \c vector of \c unique_ptr to \c IJdwpEvent objects, one for
     * each entry in \c encoded.
     *
     * @throws JdwpException if \c encoded does not represent a JDWP composite
     * event packet or if the composite event packet is malformed.
     */
    static vector<unique_ptr<IJdwpEvent>> FromComposite(const string& encoded,
        IJdwpCon& con);

    /**
     * Returns the \c JdwpEventKind of this event.
     */
    JdwpEventKind GetKind() const;
    /**
     * Reads \c encoded as a single event, including the \c eventKind byte.
     *
     * @see https://docs.oracle.com/javase/7/docs/platform/jpda/jdwp/jdwp-protocol.html#JDWP_Event
     *
     * @throws JdwpException if the \c eventKind byte in \c encoded does not
     * match the kind expected by the run-time type of \c this.
     */
    size_t FromEncoded(const string& encoded, IJdwpCon& con);
    /**
     * Dispatches \c this to the appropriate method of \c handler based on the
     * run-time type of \c this.
     */
    void Dispatch(Handler& handler);

    virtual ~IJdwpEvent() = 0;
  protected:
    virtual JdwpEventKind GetKindImpl() const = 0;
    /**
     * Implements \c FromEncoded for a given JDWP event kind.
     *
     * @param encoded The JDWP encoded fields that are are specific to a given
     * event kind (i.e., the message body without the event kind byte).
     */
    virtual size_t FromEncodedImpl(const string& encoded, IJdwpCon& con) = 0;
    virtual void DispatchImpl(Handler& handler) = 0;
};

namespace impl {

using std::tuple;

/**
 * Provides a base for types representing JDWP event packets.
 *
 * @tparam kind The JDWP command set of the command represented by this
 * packet
 * @tparam Fields A \c tuple of \c IJdwpField
 */
template <typename Derived, uint8_t kind, typename Fields>
class EventBase : public IJdwpEvent {
  public:
    Fields& GetFields() { return this->fields; }
    const Fields& GetFields() const { return this->fields; }
  protected:
    JdwpEventKind GetKindImpl() const override {
      return static_cast<JdwpEventKind>(kind);
    }
    /**
     * Provides a default implementation of \c FromEncoded. This will only
     * handle \c Fields that are just a tuple of simple \c IJdwpField, \c vector
     * fields won't work.
     */
    size_t FromEncodedImpl(const string& encoded, IJdwpCon& con) override {
      size_t curr_idx = 0;
      TupleForEach(this->fields, [&encoded, &con, &curr_idx](auto& f){
        curr_idx += f.FromEncoded(encoded.substr(curr_idx), con);
      });
      return curr_idx;
    }
    void DispatchImpl(Handler& handler) override;
  private:
    Fields fields;
};

/**
 * Defines a handler for all of the messages in the \c std::tuple \c AllMsgs
 * that share a common interface \c Interface.
 */
template<typename Interface, typename AllMsgs>
class GenericHandler;

template<typename Interface, typename NextMsgType, typename... MsgRest>
class GenericHandler<Interface, tuple<NextMsgType, MsgRest...>> :
    GenericHandler<Interface, tuple<MsgRest...>> {
  public:
    // Don't hide the rest of the handle functions in the base class
    using GenericHandler<Interface, tuple<MsgRest...>>::Handle;

    /**
     * Provides a default implementation for handling a message of \c NextMsgType.
     * By default, uses the handler for the \c Interface type. Can be overriden
     * in subclasses to customize behavior for certain messages.
     */
    virtual void Handle(NextMsgType& msg) {
      this->Handle(static_cast<Interface&>(msg));
    }
};

// Template recursion base case, no more types left in the tuple
template<typename Interface>
class GenericHandler<Interface, tuple<>> {
  public:
    /**
     * Provides a default implementation of the generic message handler. By
     * default, simply ignores the message.
     */
    virtual void Handle(Interface& msg) {
      static_cast<void>(msg);
    }
};

}  // namespace impl

namespace events {

using namespace impl;
using std::tuple;

class VmStart :
    public EventBase<
      VmStart,
      static_cast<uint8_t>(JdwpEventKind::kVmStart),
      tuple<JdwpInt, JdwpThreadId>
    > { };

class SingleStep :
    public EventBase<
      SingleStep,
      static_cast<uint8_t>(JdwpEventKind::kSingleStep),
      tuple<JdwpInt, JdwpThreadId, JdwpLocation>
    > { };

class Breakpoint :
    public EventBase<
      Breakpoint,
      static_cast<uint8_t>(JdwpEventKind::kBreakpoint),
      tuple<JdwpInt, JdwpThreadId, JdwpLocation>
    > { };

class MethodEntry :
    public EventBase<
      MethodEntry,
      static_cast<uint8_t>(JdwpEventKind::kMethodEntry),
      tuple<JdwpInt, JdwpThreadId, JdwpLocation>
    > { };

class MethodExit :
    public EventBase<
      MethodExit,
      static_cast<uint8_t>(JdwpEventKind::kMethodExit),
      tuple<JdwpInt, JdwpThreadId, JdwpLocation>
    > { };

class MethodExitWithReturnValue :
    public EventBase<
      MethodExitWithReturnValue,
      static_cast<uint8_t>(JdwpEventKind::kMethodExitWithReturnValue),
      tuple<JdwpInt, JdwpThreadId, JdwpLocation, JdwpValue>
    > { };

class MonitorContendedEnter :
    public EventBase<
      MonitorContendedEnter,
      static_cast<uint8_t>(JdwpEventKind::kMonitorContendedEnter),
      tuple<JdwpInt, JdwpThreadId, JdwpTaggedObjectId, JdwpLocation>
    > { };

class MonitorContendedEntered :
    public EventBase<
      MonitorContendedEntered,
      static_cast<uint8_t>(JdwpEventKind::kMonitorContendedEntered),
      tuple<JdwpInt, JdwpThreadId, JdwpTaggedObjectId, JdwpLocation>
    > { };

class MonitorWait :
    public EventBase<
      MonitorWait,
      static_cast<uint8_t>(JdwpEventKind::kMonitorWait),
      tuple<JdwpInt, JdwpThreadId, JdwpTaggedObjectId, JdwpLocation, JdwpLong>
    > { };

class MonitorWaited :
    public EventBase<
      MonitorWaited,
      static_cast<uint8_t>(JdwpEventKind::kMonitorWaited),
      tuple<JdwpInt, JdwpThreadId, JdwpTaggedObjectId, JdwpLocation, JdwpBool>
    > { };

class Exception :
    public EventBase<
      Exception,
      static_cast<uint8_t>(JdwpEventKind::kException),
      tuple<JdwpInt, JdwpThreadId, JdwpLocation, JdwpTaggedObjectId,
        JdwpLocation>
    > { };

class ThreadStart :
    public EventBase<
      ThreadStart,
      static_cast<uint8_t>(JdwpEventKind::kThreadStart),
      tuple<JdwpInt, JdwpThreadId>
    > { };

class ThreadDeath :
    public EventBase<
      ThreadDeath,
      static_cast<uint8_t>(JdwpEventKind::kThreadDeath),
      tuple<JdwpInt, JdwpThreadId>
    > { };

class ClassPrepare :
    public EventBase<
      ClassPrepare,
      static_cast<uint8_t>(JdwpEventKind::kClassPrepare),
      tuple<JdwpInt, JdwpThreadId, JdwpByte, JdwpReferenceTypeId, JdwpString,
        JdwpInt>
    > { };

class ClassUnload :
    public EventBase<
      ClassUnload,
      static_cast<uint8_t>(JdwpEventKind::kClassUnload),
      tuple<JdwpInt, JdwpString>
    > { };

class FieldAccess :
    public EventBase<
      FieldAccess,
      static_cast<uint8_t>(JdwpEventKind::kFieldAccess),
      tuple<JdwpInt, JdwpThreadId, JdwpLocation, JdwpByte, JdwpReferenceTypeId,
        JdwpFieldId, JdwpTaggedObjectId>
    > { };

class FieldModification :
    public EventBase<
      FieldModification,
      static_cast<uint8_t>(JdwpEventKind::kFieldModification),
      tuple<JdwpInt, JdwpThreadId, JdwpLocation, JdwpByte, JdwpReferenceTypeId,
        JdwpFieldId, JdwpTaggedObjectId, JdwpValue>
    > { };

class VmDeath :
    public EventBase<
      VmDeath,
      static_cast<uint8_t>(JdwpEventKind::kVmDeath),
      tuple<JdwpInt>
    > { };

/**
 * A tuple of all implementations of \c IJdwpEvent
 */
using AllEvents = tuple<
  VmStart,
  SingleStep,
  Breakpoint,
  MethodEntry,
  MethodExit,
  MethodExitWithReturnValue,
  MonitorContendedEnter,
  MonitorContendedEntered,
  MonitorWait,
  MonitorWaited,
  Exception,
  ThreadStart,
  ThreadDeath,
  ClassPrepare,
  ClassUnload,
  FieldAccess,
  FieldModification,
  VmDeath
  >;

}  // namespace events

/**
 * Provides a generic handler that, by default, ignores all messages. Can be
 * subclassed to provide handling functionality by overriding the various
 * \c Handle methods.
 */
class Handler : public impl::GenericHandler<IJdwpEvent, events::AllEvents> { };

template <typename Derived, uint8_t kind, typename Fields>
void impl::EventBase<Derived, kind, Fields>::DispatchImpl(Handler& handler) {
  handler.Handle(static_cast<Derived&>(*this));
}

}  // namespace roastery

#endif  // ROASTERY_JDWP_PACKET_H_

