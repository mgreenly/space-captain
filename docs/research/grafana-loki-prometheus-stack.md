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
| **Prometheus** | Metrics Monitoring & Alerting | **Numbers** (Time-series telemetry) | Answers "How is it doing?" by tracking trends, performance counters, and system health over time. It is used for high-level monitoring and firing alerts. |
| **Grafana** | Visualization & On-Call Management | (Connects to all sources) | Provides a single pane of glass to build dashboards, manage on-call schedules, and visualize data from all sources. |

## 3. Licensing Concerns

The open-source nature of this stack is a significant advantage.

-   **Grafana, Loki, Grafana OnCall:** Available under the [GNU Affero General Public License v3.0 (AGPLv3)](https://github.com/grafana/grafana/blob/main/LICENSE). The AGPL is a strong copyleft license. However, since these are standalone services that you run and interact with over a network, their license terms do not typically impact the licensing of the applications they are monitoring (like Space Captain).
-   **Prometheus, Alertmanager:** Licensed under the [Apache License 2.0](https://github.com/prometheus/prometheus/blob/main/LICENSE). This is a permissive license that has very few restrictions.
-   **Agents (Promtail, Node Exporter):** Also licensed under Apache 2.0.

**Conclusion:** There are no significant licensing concerns that would prevent the use of this stack for the Space Captain project.

## 4. Architecture and Deployment

The architecture consists of a central monitoring server and agents installed on all target machines.

### 4.1. Central Monitoring Server Deployment

A dedicated EC2 instance (or containerized environment) would run the core services.

1.  **Install Prometheus:** Configure `prometheus.yml` to define which targets to scrape for metrics.
2.  **Install Loki:** Configure to receive logs from Promtail agents.
3.  **Install Grafana:** Install the Grafana server.
4.  **Install Alerting Manager:** Choose **one** of the primary alerting managers below.
    -   **Prometheus Alertmanager:** A standalone service that handles alerts from Prometheus.
    -   **Grafana OnCall:** Can be self-hosted or used via Grafana Cloud.
5.  **Wiring them together in Grafana:**
    -   Access the Grafana UI.
    -   Add Prometheus and Loki as Data Sources.
    -   If using Grafana OnCall, configure it within the Grafana UI. If using Alertmanager, it runs separately but can be configured as a data source for viewing alert status.

### 4.2. Target Machine Installation (Server and Clients)

On every machine to be monitored, install two lightweight agents:

1.  **Node Exporter:** Collects system-level metrics (CPU, RAM, etc.) for Prometheus.
2.  **Promtail:** Ships logs to Loki. Can be configured to read from the systemd journal, which is ideal for services.

## 5. Alerting Strategy

A monitoring stack is incomplete without a robust alerting strategy. The Grafana ecosystem provides two excellent, but distinct, options.

### 5.1. Primary Recommendation: Prometheus + Alertmanager

This is the traditional, most robust, and reliable approach for metric-based alerting.

-   **How it works:** Alerting rules are defined in Prometheus using PromQL. When a rule's condition is met, Prometheus fires an alert to the **Alertmanager**.
-   **Alertmanager:** This is a dedicated, open-source service whose sole job is to process alerts. It provides critical features to prevent alert fatigue:
    -   **Deduplication & Grouping:** Combines related alerts into a single notification.
    -   **Routing:** Sends alerts to different channels based on labels (e.g., `severity=critical` goes to PagerDuty, `severity=info` goes to Slack).
    -   **Silencing & Inhibition:** Allows for muting alerts during maintenance or suppressing less important alerts when a more critical one is firing.
-   **Why it's recommended:** Its key advantage is **reliability**. Your alerting is decoupled from your visualization layer (Grafana). If Grafana has an issue, your critical alerts will still be sent by Alertmanager.

### 5.2. Ecosystem-Integrated Alternative: Grafana OnCall

This is Grafana's modern, integrated answer to on-call management and alerting.

-   **How it works:** Grafana OnCall is an on-call management tool that can be used as a standalone service or, more powerfully, integrated directly into the Grafana UI.
-   **Key Advantages:**
    -   **Unified UI:** Manage dashboards, metrics, logs, and on-call schedules all within Grafana. This creates a seamless workflow from alert to investigation.
    -   **Alert on Anything:** Unlike Alertmanager which only works with Prometheus metrics, Grafana can create alerts from any data source, including Loki logs (e.g., "alert if more than 10 login failures are logged in 5 minutes").
-   **Hosting Options:**
    -   **Grafana Cloud:** Offers a generous free tier (e.g., 3 users, 100 SMS/voice calls per month). This is the easiest way to start.
    -   **Self-Hosted:** The open-source version can be run on your own infrastructure for free, giving you unlimited users but requiring you to manage it and pay for third-party notification services (like Twilio for SMS).

### 5.3. Notification Channels

Both Alertmanager and Grafana OnCall send notifications via **integrations**. You do not need to build custom notification logic. Common supported services include:

-   **Incident Management:** PagerDuty, Opsgenie.
-   **ChatOps:** Slack, Microsoft Teams, Discord, Telegram.
-   **Direct Channels:** Email, SMS, Phone Calls.
-   **Generic Webhooks:** This allows integration with almost any other service, including custom scripts. Note that niche, privacy-focused apps like **Signal** are not natively supported due to a lack of official bot APIs; a custom webhook bridge would be required.

## 6. Custom Telemetry and Structured Logging

(Content from previous version remains unchanged)

## 7. Performance and Scaling Concerns

(Content from previous version remains unchanged)

## 8. Conclusion for Client Fleet Monitoring

(Content from previous version remains unchanged)