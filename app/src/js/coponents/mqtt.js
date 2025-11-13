/**
 * MQTT Status Monitor
 * Listens to WebSocket messages to monitor MQTT connection status in real-time
 */
export default {
    init() {
        // Listen for WebSocket changes to update MQTT status
        window.addEventListener('wschanged', (event) => {
            try {
                const data = JSON.parse(event.detail);
                if (data.mqtt && typeof data.mqtt.connected !== 'undefined') {
                    const mqttStore = Alpine.store('mqtt');
                    mqttStore.connected = data.mqtt.connected;
                }
            } catch (error) {
                console.warn('Could not parse WebSocket message for MQTT status:', error);
            }
        });

        // Also try to get initial status from /status endpoint if WebSocket hasn't sent data yet
        this.checkInitialStatus();
    },

    async checkInitialStatus() {
        try {
            const response = await fetch('/status');
            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            const data = await response.json();

            // Only update if we haven't received data from WebSocket yet
            const mqttStore = Alpine.store('mqtt');
            if (mqttStore.connected === null && data.mqtt && typeof data.mqtt.connected !== 'undefined') {
                mqttStore.connected = data.mqtt.connected;
            }
        } catch (error) {
            console.warn('Could not fetch initial MQTT status:', error);
        }
    }
};
