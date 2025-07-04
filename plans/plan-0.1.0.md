# Space Captain v0.1.0 Implementation Plan

This plan breaks down the version 0.1.0 PRD into actionable, testable steps with integrated Ruby testing infrastructure at each phase.

## Key Principles

1. **Separation of Concerns**: Ruby test harness is completely separate from C server
2. **Negative Testing**: Easily create malformed packets and protocol attacks
3. **Scalability**: Use Ruby's async capabilities for thousands of concurrent clients
4. **No Contamination**: Pure C server, Ruby is just a sophisticated testing tool

## Phase 0: Foundation & Testing Framework

### Build Steps
1. Create config files (server.conf, client.conf) with basic settings
2. Create debug logging framework with levels and timestamps
3. Create Ruby test harness base with socket and async support (using Async gem)

### Test Infrastructure
- Ruby scripts can send arbitrary UDP packets for testing
- Quick setup with `gem install async async-io`
- Clean DSL for test scenarios
- Easy binary data manipulation with `pack/unpack`

---

## Phase 1: Basic UDP Communication & Robustness Testing

### Build Steps
4. Define protocol message structures and headers
5. Server: UDP socket creation with epoll loop
6. **Test:** Ruby script verifies server receives and logs raw UDP packets
7. Client: Basic UDP send functionality
8. **Test:** Ruby malformed packet tests (invalid headers, wrong lengths)

### Testing Focus
- Simple echo test - client sends packet, server echoes back
- Negative testing with malformed data:
  ```ruby
  # Send garbage data
  sock.send("\xDE\xAD\xBE\xEF", 0, server_addr)
  
  # Send oversized packet
  sock.send("A" * 65536, 0, server_addr)
  
  # Send empty packet
  sock.send("", 0, server_addr)
  ```

---

## Phase 2: Connection Handshake with Concurrent Testing

### Build Steps
9. Server: Implement 4-way handshake state machine (HELLO/CHALLENGE/RESPONSE/WELCOME)
10. Client: Implement handshake participation
11. **Test:** Ruby async handshake test with 100+ concurrent clients

### Testing Focus
- Multiple clients can connect, each gets unique session token and entity_id
- Concurrent connection testing:
  ```ruby
  Async do |task|
    100.times do
      task.async { simulate_client_handshake }
    end
  end
  ```

---

## Phase 3: Security & Session Management

### Build Steps
12. Server: Session tracking with token validation
13. **Test:** Ruby negative tests - invalid tokens, spoofed sessions
14. Server: 30-second timeout mechanism
15. Client: Heartbeat sending every 15 seconds
16. **Test:** Ruby timeout test with controlled heartbeat timing

### Testing Focus
- Client disconnects if no heartbeat sent
- Server removes timed-out clients
- Security attack simulations:
  - Token replay attacks
  - Session hijacking attempts
  - Heartbeat flooding

---

## Phase 4: Reliable State Updates

### Build Steps
17. Server: Add sequence numbers to dummy state packets (4Hz)
18. Client: Receive state and send ACKs
19. **Test:** Ruby packet disorder test - inject out-of-sequence packets
20. Server: Dual state buffers (current/last acknowledged)
21. Debug: Add server state inspection endpoint/command
22. **Test:** Ruby script queries debug endpoint to verify state buffers

### Testing Focus
- Simulate packet reordering/duplication
- Verify client processes newest state only
- Packet disorder testing:
  ```ruby
  # Send packets 5, 3, 4, 1, 2
  packets.shuffle.each { |pkt| sock.send(pkt, 0, addr) }
  ```

---

## Phase 5: Multi-threaded Architecture

### Build Steps
23. Server: Create worker thread pool with MPSC queues
24. **Test:** Ruby queue stress test - bombard with rapid messages
25. Server: Implement 128x128 Hilbert curve mapping
26. Server: Double-buffered partition maps with atomic swap
27. Debug: Add partition map visualization/logging
28. **Test:** Ruby script monitors partition rebalancing via debug interface
29. Server: Route messages to workers based on entity location
30. Server: Workers send state updates at 4Hz
31. **Test:** Ruby async test with 500+ clients across all workers

### Testing Focus
- 100+ clients connect; verify all workers process messages at 4Hz
- Partition visualization and monitoring
- Concurrency stress testing with controlled load

---

## Phase 6: Game World

### Build Steps
32. Game: Define entity structure (id, position, heading, hull, power dials)
33. Server: Implement consistent snapshot for rebalancing
34. **Test:** Ruby race condition test during snapshot capture

### Testing Focus
- Ships spawn correctly at 1.5e11m from origin
- Grid assignments update smoothly under load
- Snapshot consistency verification

---

## Phase 7: Client UI

### Build Steps
35. Client: Initialize ncurses with layout (map/status/REPL)
36. Client: Display own ship state from received packets
37. **Test:** Ruby automated UI verification (capture/compare output)

### Testing Focus
- Client shows position, heading, hull, power dials updating at 4Hz
- Visual verification of UI layout
- Automated output comparison

---

## Phase 8: Movement System

### Build Steps
38. Game: Movement physics with logarithmic speed (1-1000)
39. Client: Parse and send movement commands
40. **Test:** Ruby physics validation - verify distance/time calculations

### Testing Focus
- Ships move at correct speeds
- Positions update smoothly
- Physics calculations verification

---

## Phase 9: Area of Interest

### Build Steps
41. Server: Area of Interest filtering (5e10m radius)
42. Client: Display nearby entities on map
43. **Test:** Ruby bandwidth measurement - compare with/without AoI

### Testing Focus
- Client only receives updates for ships within 5e10m
- Bandwidth usage reduction measurement
- Edge case testing at AoI boundaries

---

## Phase 10: Combat System

### Build Steps
44. Game: Weapon firing with cross-worker damage routing
45. Server: Implement DamageCommand via MPSC queues
46. **Test:** Ruby cross-partition combat test with timing verification
47. Client: Combat commands and damage display

### Testing Focus
- Ships can fire, take damage, and be destroyed
- Cross-partition messaging verification
- Combat timing and synchronization

---

## Phase 11: Production Readiness

### Build Steps
48. Server: Graceful shutdown (SIGINT/SIGTERM)
49. **Test:** Ruby shutdown sequence test - verify all clients notified

### Testing Focus
- Server reads config correctly
- Clean shutdown with connected clients
- All resources properly released

---

## Phase 12: Performance Validation

### Build Steps
50. **Perf:** Ruby async stress test simulating 1000+ clients
51. **Perf:** Ruby script to generate protocol fuzzing attacks
52. **Perf:** Run server under valgrind with Ruby load generator
53. **Perf:** Ruby latency measurement harness with statistics
54. **Docs:** Write README with build/run/test instructions

### Testing Focus
- Meet all PRD success criteria:
  - 1000+ concurrent clients
  - Stable 4Hz tick rate
  - <50ms LAN latency
  - Zero memory leaks (24-hour test)
  - <1% packet loss

### Ruby Fuzzing Example:
```ruby
# Generate random valid-looking but malformed packets
1000.times do
  header = [rand(256), rand(65536)].pack("CL")
  body = Random.bytes(rand(1000))
  sock.send(header + body, 0, server_addr)
end
```

---

## Ruby Testing Infrastructure Benefits

1. **Development Speed**: Write complex test scenarios in hours vs weeks
2. **Malformed Data**: Trivial to send arbitrary bytes for robustness testing
3. **Async Scalability**: Single Ruby process can simulate thousands of clients
4. **Clean Separation**: C server remains pure, Ruby is just a testing tool
5. **Rich Ecosystem**: Use RSpec, Minitest, or custom frameworks
6. **Interactive Debugging**: Use IRB/Pry for live test exploration

The Ruby test harness acts as a sophisticated, programmable client that can simulate both normal operation and adversarial conditions, ensuring the C server is robust and performant.

## Success Metrics

Each phase produces a working system that can be tested before proceeding. The plan ensures:
- Incremental complexity building on solid foundations
- Comprehensive testing at each step
- Early detection of architectural issues
- Confidence in system robustness before adding features