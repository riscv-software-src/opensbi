OpenSBI Domain Bridge Support
==============================

OpenSBI domain bridge support provides a configured message path between
domains using SBI MPXY channels. A bridge connects one destination domain to
one or more source domains. Sources submit requests through their MPXY
channels, and the destination domain handles the forwarded requests.

A bridge describes how the participating domains and MPXY channels are
connected. Protocol-specific source nodes identify the type of requests being
forwarded.

Bridge instance
-------------------

A bridge instance is represented by **struct mpxy_bridge** in OpenSBI and
has the following details:

* **node** - List node used to link this bridge instance into a list
* **nodeoff** - FDT node offset of this bridge
* **dst** - Destination endpoint configuration and runtime state
* **sources** - Array of source endpoint instances configured for this bridge
* **source_count** - Number of source instances in the sources array
* **store** - Per-HART storage for tracking bridge communication state

A bridge endpoint is represented by **struct mpxy_bridge_endpoint** and
has the following details:

* **domain_name** - Name of the domain associated with this endpoint
* **channel_id** - MPXY channel ID associated with this endpoint
* **dom** - Pointer to the resolved OpenSBI domain instance
* **ch** - MPXY channel associated with this endpoint
* **rpmi_endpoint** - RPMI endpoint data associated with this endpoint

RPMI endpoint data is represented by **struct bridge_rpmi_endpoint** and
has the following details:

* **rpmi_data** - RPMI data associated with this endpoint
* **bridge** - Pointer to the bridge instance containing this endpoint
* **domain** - Pointer to the endpoint's domain
* **channel** - Pointer to the endpoint's MPXY channel

A source instance is represented by **struct mpxy_bridge_source** and
has the following details:

* **nodeoff** - FDT node offset of this source
* **ep** - Endpoint configuration and runtime state for this source
* **ops** - Operations provided by the source driver

Per-HART bridge storage is represented by **struct mpxy_bridge_slot_store**
and has the following details:

* **slots** - Array of per-HART bridge slots
* **hart_count** - Number of HART slots in the array

Each per-HART slot is represented by **struct mpxy_bridge_hart_slot** and
has the following details:

* **active_source** - Index of the source currently active for this HART
* **src_ctx** - Source-side bridge context for this HART
* **dst_resp** - Destination-side response state for this HART

## Bridge model

A bridge consists of:

- **Destination** — the domain that receives and handles forwarded requests.
  The bridge configures a destination MPXY channel in this domain.
- **Sources** — one or more endpoints that submit requests. Each source is
  associated with a source domain and an MPXY channel.
- **Source type** — the source node’s `compatible` property identifies the
  protocol or service associated with that source.

At a high level, a source sends a request through its MPXY channel, the bridge
forwards it to the destination domain, and the response is returned to the
source.

## Device Tree binding

The bridge is represented by a parent node and one or more child source nodes.

### Bridge node

The bridge node has the following properties:

- `compatible` (required) — identifies the bridge implementation. For an
  RPMI request-forwarding bridge, use
  `"opensbi,rpmi-mpxy-reqfwd-bridge"`.
- `riscv,domain` (required) — phandle of the destination domain.
- `opensbi,destination-channel-id` (required) — MPXY channel ID used by the
  destination.

### Source node

Each source is a child of the bridge node and has the following properties:

- `compatible` (required) — identifies the source protocol or service.
- `riscv,domain` (required) — phandle of the source domain.
- `opensbi,source-channel-id` (required) — MPXY channel ID used by the source.

A bridge may contain multiple source nodes. Each source has its own
compatible string, domain, and channel ID. Source and destination domains
configured for a bridge must have identical possible-hart sets.

## Example

```dts
bridge0: untrusted-to-trusted-bridge {
    compatible = "opensbi,rpmi-mpxy-reqfwd-bridge";
    riscv,domain = <&tdomain>;
    opensbi,destination-channel-id = <0x2>;

    source0 {
        compatible = "opensbi,rpmi-mpxy-reqfwd-mm";
        riscv,domain = <&udomain>;
        opensbi,source-channel-id = <0x1>;
    };

    source1 {
        compatible = "opensbi,rpmi-mpxy-reqfwd-tee";
        riscv,domain = <&udomain>;
        opensbi,source-channel-id = <0x3>;
    };
};
