# Final Iterative Work Plan for Space Captain v0.1.0

This document presents the final, restructured work plan for implementing Space Captain v0.1.0. It is designed around iterative development, ensuring that each phase delivers a testable, integrated piece of functionality, mitigating risks and improving the development feedback loop.

---

### **Iteration 1: Foundation & Test Harness (Weeks 1-2)**

*   **Goal:** Establish the project foundation and a minimal, testable server loop.
*   **Verifiable Outcome:** A single-threaded server skeleton runs, compiles cleanly, and passes its first unit tests. A mock test client can connect and receive a response.
*   **Tasks:**
    *   [ ] **1.1: Project Scaffolding:** Set up the `Makefile`, directory structure, and Git repository.
    *   [ ] **1.2: Core Utilities:** Implement the centralized `config.h` and the thread-safe logging system.
        *   `config.h` will explicitly define key constants from the PRD: `AOI_RADIUS (5.0e10)`, `SPAWN_RADIUS (1.5e11)`, and `WORLD_RADIUS (1.0e14)`.
    *   [ ] **1.3: Testing Framework Integration:** Integrate the Unity test framework and create the initial test runner.
    *   [ ] **1.4: Basic Server Loop:** Implement a non-blocking UDP socket with a basic `epoll` loop and graceful signal handling.
    *   [ ] **1.5: Minimal Test Client:** Create a `tests/mock_client.c` capable of sending a single packet and asserting a response, to be used in automated functional tests.

---

### **Iteration 2: Secure Connection & Protocol Basics (Weeks 3-4)**

*   **Goal:** Implement a secure communication channel and the basic message-passing system.
*   **Verifiable Outcome:** The mock test client can establish a full DTLS 1.2 connection, send a `PING` message, and receive a `PONG` response, all verified with a functional test.
*   **Tasks:**
    *   [ ] **2.1: DTLS Integration:** Implement the security abstraction layer, certificate management, and client-side pinning.
        *   Generate a self-signed server certificate for the server to use.
        *   Configure the DTLS context to prioritize performant cipher suites, specifically `AES-GCM`.
    *   [ ] **2.2: Secure Handshake:** Integrate the DTLS handshake into the main server loop for new connections.
        *   Implement a 30-second timeout for the handshake and for ongoing connections that have not sent a heartbeat.
    *   [ ] **2.3: Core Protocol:** Define the `MessageHeader` and a basic `PING`/`PONG` message set. Implement the necessary serialization/deserialization.
    *   [ ] **2.4: Functional Connection Test:** Create `tests/connection_test.c` which uses the mock client to automate and verify the secure connection and ping/pong exchange.

---

### **Iteration 3: Concurrent Architecture & Initial Client (Weeks 5-7)**

*   **Goal:** Build the concurrent worker architecture and a minimal, interactive `ncurses` client that can connect and see itself.
*   **Verifiable Outcome:** The server runs with the 32-thread worker pool. A basic `ncurses` client can connect, be assigned an entity ID, and see its own ship rendered on screen based on server state updates.
*   **Tasks:**
    *   [ ] **3.1: Concurrent Primitives:** Implement and unit-test the lock-free MPSC queue.
    *   [ ] **3.2: Worker Pool:** Implement the worker pool, the distributed tick foundation (with synchronization barrier), and the round-robin assignment of clients.
        *   New connections will be flagged as "pending" upon initial round-robin assignment. This flag will be cleared after the first rebalance cycle places them in a definitive worker thread.
    *   [ ] **3.3: Initial State Management:** Define the `Ship` struct. Implement the `CONNECTION_ACCEPTED` and a minimal `STATE_UPDATE` message (containing only the player's own ship).
    *   [ ] **3.4: Minimal `ncurses` Client:** Create the initial `src/client.c` project. Implement the client-side networking and a basic `ncurses` UI that connects to the server and renders the player's ship based on `STATE_UPDATE` messages.

---

### **Iteration 4: Full Gameplay & Client Interactivity (Weeks 8-10)**

*   **Goal:** Implement all core gameplay mechanics and make the client fully interactive.
*   **Verifiable Outcome:** The `ncurses` client can control ship movement and power dials, fire weapons, see other players within its AoI, and witness combat and destruction in real-time.
*   **Tasks:**
    *   [ ] **4.1: Full Protocol Implementation:** Implement all remaining C2S/S2C messages (`DIAL_UPDATE`, `FIRE_WEAPON`, `ENTITY_DESTROYED`, etc.).
    *   [ ] **4.2: Gameplay Logic:** Implement the server-side logic for movement, combat, damage, and shield absorption.
    *   [ ] **4.3: Multi-Entity State:** Expand `STATE_UPDATE` to include all entities within the Area of Interest.
        *   The `STATE_UPDATE` message will use a "dummy diff" implementation where all fields are always marked as changed, effectively sending the full state of visible entities each tick, as specified for v0.1.0.
    *   [ ] **4.4: Full Client Interactivity:** Update the `ncurses` client to handle all user input (dials, firing, movement) and render all entities and notifications from the server.
    *   [ ] **4.5: Respawn Loop:** Implement the full destruction and reconnection/respawn flow on both server and client.

---

### **Iteration 5: Advanced Features & Optimization (Weeks 11-12)**

*   **Goal:** Integrate complex backend optimizations and client-side enhancements.
*   **Verifiable Outcome:** The server now uses Hilbert curve partitioning for load balancing. The client experience is smoother thanks to client-side prediction.
*   **Tasks:**
    *   [ ] **5.1: Spatial Partitioning:** Implement the Hilbert curve grid, the rebalancing thread, and the lock-free partition map swapping.
        *   The rebalancing thread will acquire a "consistent snapshot" of all entity positions at the start of a tick to ensure an accurate load assessment without data races.
    *   [ ] **5.2: Cloaking Mechanics:** Implement the server-side cloak detection formula and integrate it into the AoI calculation.
    *   [ ] **5.3: Client-Side Prediction:** Implement position extrapolation and snap-correction in the `ncurses` client.
    *   [ ] **5.4: Initial Profiling:** Use `perf` with multiple simulated clients to identify and fix any obvious performance hotspots.

---

### **Iteration 6: Hardening & Final Validation (Weeks 13-14)**

*   **Goal:** Ensure the system is reliable, robust, and meets all performance and portability requirements.
*   **Verifiable Outcome:** The server passes a 24-hour, 2,000-client stress test with zero memory leaks and maintains the 4 Hz tick rate. All functional tests pass on both `x86_64` and `AArch64`.
*   **Tasks:**
    *   [ ] **6.1: Reliability Protocols:** Implement the `HEARTBEAT`, `ERROR_RESPONSE`, and `DISCONNECT_NOTIFY` protocols to ensure connection stability and graceful error handling.
    *   [ ] **6.2: Large-Scale Load Testing:** Develop a lightweight script to simulate 2,000+ concurrent connections and run sustained load tests.
    *   [ ] **6.3: Memory & Concurrency Validation:** Run 24-hour tests using `valgrind` to detect memory leaks and `TSan` to find data races.
    - [ ] **6.4: Cross-Platform Validation:** Compile and run the entire test suite on an `AArch64` environment to verify portability.
    - [ ] **6.5: Final PRD Review:** Systematically verify every success criterion listed in the PRD.

---

### **Addendum: Evaluation of the Order of Events**

The iterative structure of this plan is a significant improvement over a more linear, waterfall-style approach. It is considered highly optimal for the following reasons:

1.  **De-risking Technical Complexity:** The plan tackles the hardest and most foundational problems first.
    *   **Iteration 1 (Foundation):** Establishes the build and test harness, which is essential for all subsequent work.
    *   **Iteration 2 (Security):** Front-loads the DTLS security layer. This is a critical, complex piece of the infrastructure that everything else depends on. Getting it right early is crucial.
    *   **Iteration 3 (Concurrency):** Addresses the lock-free queues and worker pool. This is arguably the most complex part of the C implementation, and building it before adding heavy game logic allows for focused testing and validation of the concurrent architecture itself.

2.  **Early, Continuous Integration:** The plan is designed to have a "runnable" and "testable" artifact at the end of each iteration.
    *   By **Iteration 3**, there is a minimal but functional `ncurses` client that can connect to the multi-threaded server. This is a massive milestone. It proves that the core architecture, from networking to concurrency, is working in an integrated fashion.
    *   This avoids the "big bang" integration at the end of the project, where all components are brought together for the first time, which is a common source of failure and delays.

3.  **Excellent Feedback Loop:** The introduction of the client in Iteration 3 and its full implementation in Iteration 4 creates a powerful feedback loop.
    *   Developers can **play the game** to test new features. This is often more effective at finding bugs and design flaws than purely automated tests. For example, issues with movement logic or combat feel might not be obvious from unit tests but become immediately apparent during gameplay.
    *   This iterative "build-a-feature, play-the-feature" cycle makes development more engaging and effective.

4.  **Logical Dependency Management:** The phases flow in a natural order of dependency.
    *   You can't have a protocol without a secure connection.
    *   You can't have game logic without a concurrent architecture to run it on.
    *   You can't have a fully interactive client without the gameplay logic and protocol being in place on the server.
    *   Complex optimizations like spatial partitioning are correctly deferred until *after* the core system is proven to work, treating them as an enhancement rather than a foundational blocker.

5.  **Dedicated Hardening Phase:** The final iteration is rightly focused on stability, performance, and validation. By this point, all features are complete, and the team can focus exclusively on meeting the non-functional requirements (performance, reliability, portability) laid out in the PRD's success criteria.

---

### **Addendum 2: Evaluation of Testable Outcomes per Iteration**

The plan's quality is significantly enhanced by ensuring each iteration produces a concretely testable outcome, allowing progress to be measured and verified.

*   **Iteration 1: Foundation & Test Harness**
    *   **Testability:** Excellent. `make` provides compilation pass/fail. `make tests` validates core utilities. A functional test (`tests/core_loop_test.c`) can automate sending a single UDP packet via a mock client and assert that the server's `epoll` loop receives it, providing an end-to-end test of the server skeleton.

*   **Iteration 2: Secure Connection & Protocol Basics**
    *   **Testability:** Excellent. A new functional test (`tests/secure_protocol_test.c`) serves as the primary integration test. It automates the full DTLS handshake and a `PING`/`PONG` message exchange, providing a definitive pass/fail on the entire security and basic message-passing stack.

*   **Iteration 3: Concurrent Architecture & Initial Client**
    *   **Testability:** Excellent. This iteration combines automated and manual testing. The MPSC queue has its own multi-threaded unit test. The primary test, however, is manual: running `bin/client` to visually confirm that a player can connect and see their ship. This is the first "proof of life" for the game itself.

*   **Iteration 4: Full Gameplay & Client Interactivity**
    *   **Testability:** Excellent. Testing shifts to interactive use cases. The most effective test is running two client instances to verify multi-user interaction (combat, visibility). These manual scenarios can then be scripted into automated functional tests that assert correct state changes (e.g., hull damage) after specific actions.

*   **Iteration 5: Advanced Features & Optimization**
    *   **Testability:** Good. The tests are more complex. Spatial partitioning can be tested by instrumenting the server to dump worker assignments for verification against expected patterns. Client-side prediction is primarily a visual test, observing smoothness under normal and simulated high-latency conditions.

*   **Iteration 6: Hardening & Final Validation**
    *   **Testability:** Excellent. This entire iteration is a suite of pass/fail tests. The load test has clear metrics (24 hours, 2000 clients, 4 Hz tick rate). `valgrind` and `TSan` produce definitive reports. Cross-platform validation is a clear pass/fail based on running the test suite on `AArch64`.
