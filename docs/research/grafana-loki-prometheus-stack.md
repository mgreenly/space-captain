# Research: Monitoring with Grafana, Loki, and Prometheus

This document outlines a recommendation for a comprehensive monitoring and observability stack for the Space Captain project, including the server and a fleet of clients. The recommended stack consists of Grafana, Loki, and Prometheus.

## 1. Overview and Recommendation

For a project involving a custom server, a fleet of clients, and the need to capture both logs and custom telemetry, a robust, scalable, and cost-effective monitoring solution is critical. The combination of Grafana, Loki, and Prometheus is an industry-standard, open-source solution that is exceptionally well-suited for this task.

-   **Grafana:** The visualization layer. A powerful dashboarding and analytics tool that can connect to multiple data sources simultaneously.
-   **Loki:** The log aggregation system. It is designed to be highly efficient and cost-effective for storing and querying log data.
-   **Prometheus:** The metrics monitoring system. It is the de-facto standard for collecting, storing, and querying time-series numerical data (telemetry).

This stack provides a unified view of the entire system, allowing developers to correlate system health metrics directly with application logs, which is invaluable for debugging and performance analysis.

## 2. Component Roles

It is essential to understand the distinct, complementary roles of each component:

| Component | Primary Role | Data Type | Use Case |
| :--- | :--- | :--- | :--- |
| **Loki** | Log Aggregation | **Text** (Log lines, events) | Answers "What happened?" by allowing you to search and inspect specific events, error messages, and stack traces. |
| **Prometheus** | Metrics Monitoring | **Numbers** (Time-series telemetry) | Answers "How is it doing?" by tracking trends, performance counters, and system health over time. It is used for high-level monitoring and alerting. |
| **Grafana** | Visualization | (Connects to Loki & Prometheus) | Provides a single pane of glass to build dashboards that display both logs from Loki and metrics from Prometheus, correlating them by time and labels. |

## 3. Licensing Concerns

The open-source nature of this stack is a significant advantage.

-   **Grafana:** Available under the [GNU Affero General Public License v3.0 (AGPLv3)](https://github.com/grafana/grafana/blob/main/LICENSE). The AGPL is a strong copyleft license. However, since Grafana is a standalone service that you run and interact with over a network, its license terms do not typically impact the licensing of the applications it is monitoring (like Space Captain).
-   **Loki:** Licensed under the [AGPLv3](https://github.com/grafana/loki/blob/main/LICENSE). Like Grafana, it is a separate service, and its license does not affect your project.
-   **Prometheus:** Licensed under the [Apache License 2.0](https://github.com/prometheus/prometheus/blob/main/LICENSE). This is a permissive license that has very few restrictions.
-   **Agents (Promtail, Node Exporter):** Also licensed under Apache 2.0.

**Conclusion:** There are no significant licensing concerns that would prevent the use of this stack for the Space Captain project. The AGPL-licensed components are run as separate services and do not impose any licensing requirements on the Space Captain codebase itself.

## 4. Architecture and Deployment

The architecture consists of a central monitoring server and agents installed on all target machines (both the server and the clients).

### 4.1. Central Monitoring Server Deployment

You would typically set up a dedicated EC2 instance (or a containerized environment) to run the core monitoring services.

1.  **Install Prometheus:**
    -   Download the latest Prometheus release and configure its `prometheus.yml` file.
    -   This configuration will define which "targets" Prometheus should scrape for metrics. These targets will be the IP addresses and ports of your Space Captain server and all clients.
2.  **Install Loki:**
    -   Download and run Loki. Its configuration is generally simple for a single-node setup. It needs to be configured to listen for log data from Promtail agents.
3.  **Install Grafana:**
    -   Install and start the Grafana server.
    -   **Wiring them together:**
        1.  Access the Grafana web UI.
        2.  Navigate to "Configuration" -> "Data Sources".
        3.  Add a new "Prometheus" data source, pointing it to your local Prometheus server's URL (e.g., `http://localhost:9090`).
        4.  Add a new "Loki" data source, pointing it to your local Loki server's URL (e.g., `http://localhost:3100`).

### 4.2. Target Machine Installation (Server and Clients)

On every machine you want to monitor (both the EC2 instance running `sc-server` and all machines running `sc-client`), you need to install two lightweight agents:

1.  **Node Exporter:**
    -   This is a standard Prometheus exporter that collects system-level metrics (CPU, RAM, disk I/O, network stats).
    -   You simply run it on the machine. Prometheus will be configured to scrape the `/metrics` endpoint that it exposes (typically on port 9100).
2.  **Promtail:**
    -   This is the Loki agent responsible for shipping logs.
    -   You configure its `config.yml` to tell it where your Loki server is.
    -   Crucially, you configure its `scrape_configs` section to find and process your application's logs. For a systemd service, this is particularly clean:

    ```yaml
    server:
      http_listen_port: 9080
      grpc_listen_port: 0

    positions:
      filename: /tmp/positions.yaml

    clients:
      - url: http://<your-loki-server-ip>:3100/loki/api/v1/push

    scrape_configs:
    - job_name: systemd-journal
      journal:
        max_age: 12h
        labels:
          job: space-captain
      relabel_configs:
        - source_labels: ['__journal__systemd_unit']
          target_label: 'unit'
    ```

This configuration will automatically collect logs from the systemd journal for all units and send them to Loki with appropriate labels.

## 5. Custom Telemetry and Structured Logging

This is where the real power of the stack comes into play. You should **not** put custom metrics in JSON-formatted logs. Logs are for events; metrics are for measurements.

### 5.1. Exposing Custom Telemetry with Prometheus

The correct way to provide custom telemetry is to have your application expose them in the Prometheus exposition format over a simple HTTP endpoint.

1.  **Instrument the Application:** You would need to add a lightweight HTTP server to your C application (e.g., using a library like `libmicrohttpd`) that exposes a `/metrics` endpoint.
2.  **Format the Metrics:** The format is a simple text-based key-value format.

    ```
    # HELP sc_active_connections The current number of active connections.
    # TYPE sc_active_connections gauge
    sc_active_connections 42

    # HELP sc_messages_processed_total The total number of messages processed.
    # TYPE sc_messages_processed_total counter
    sc_messages_processed_total 1337
    ```

    -   `# HELP`: A comment describing the metric.
    -   `# TYPE`: The metric type (`gauge` for values that go up and down, `counter` for values that only increase).
    -   The final line is the metric name, optional labels, and the current value.

3.  **Configure Prometheus to Scrape:** You would add a new job to your `prometheus.yml` to tell it to scrape the `/metrics` endpoint of your server and clients.

### 5.2. Structured Logging for Loki

While metrics go to Prometheus, you can make your logs much more useful in Loki by making them structured. This allows Loki to parse them and make their key-value pairs available for filtering.

A good practice is to use the `logfmt` style, which is simple and human-readable.

**Example C Log Output:**

Instead of:
`log_error("Connection failed for user 123: timeout");`

Which produces:
`[ERROR] Connection failed for user 123: timeout`

You would change your logging function to produce:
`level=error msg="Connection failed" user_id=123 reason=timeout`

**Promtail Configuration:**

You can then configure Promtail to parse this format using a `pipeline_stages` block. This extracts `level`, `user_id`, and `reason` as internal labels you can query on in Grafana, without having to use expensive regex queries on the log line itself.

```yaml
scrape_configs:
- job_name: space-captain
  journal:
    # ...
  pipeline_stages:
    - logfmt:
        mapping:
          level:
          msg:
          user_id:
          reason:
```

## 6. Performance and Scaling Concerns

This stack is used by massive companies and scales well, but there are things to be aware of.

-   **Loki:**
    -   **Label Cardinality:** Performance is highly dependent on the "cardinality" of your labels. Avoid using labels with unbounded values (like `user_id` or `trace_id`). Use labels for a small, finite set of values (`app`, `environment`, `level`). High cardinality will slow down Loki's index.
    -   **Horizontal Scaling:** For very high log volume, Loki can be scaled out horizontally by running its components (ingester, distributor, querier) as separate microservices.

-   **Prometheus:**
    -   **Time Series Cardinality:** Similar to Loki, performance depends on the number of unique time series (the combination of a metric name and its label set).
    -   **Scrape Interval:** More frequent scraping uses more resources on both Prometheus and the target machines.
    -   **Long-Term Storage:** A single Prometheus instance is not meant for perpetual storage. For long-term retention, you would integrate a solution like **Thanos** or **Cortex**.

-   **Grafana:**
    -   Grafana is generally lightweight. Performance issues typically only arise with a very large number of concurrent users or extremely complex dashboards querying massive amounts of data. It can be scaled horizontally if needed.

## 7. Conclusion for Client Fleet Monitoring

All the principles described above apply equally to the fleet of client processes. Each client machine would run:
1.  **Node Exporter** for system metrics.
2.  **Promtail** to ship its logs to the central Loki instance.
3.  The `sc-client` process itself would expose a `/metrics` endpoint with client-specific telemetry (e.g., `time_to_connect_ms`, `server_rtt_ms`).

Prometheus would be configured to scrape all client endpoints. In Grafana, you could then build dashboards that aggregate data from the entire fleet or drill down into a single misbehaving client, seamlessly switching between its metrics and logs.
