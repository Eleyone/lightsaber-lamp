/**
 * MQTT Status Monitor
 * Polls the /status endpoint to monitor MQTT connection status
 */
export default {
    checkInterval: null,
    async checkMqttStatus() {
        try {
            const response = await fetch('/status');
            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            const data = await response.json();

            // Update the mqtt store with connection status
            const mqttStore = Alpine.store('mqtt');
            if (data.mqtt && typeof data.mqtt.connected !== 'undefined') {
                mqttStore.connected = data.mqtt.connected;
            } else {
                // If MQTT data is not available, mark as unknown
                mqttStore.connected = null;
            }
        } catch (error) {
            console.error('Error checking MQTT status:', error);
            const mqttStore = Alpine.store('mqtt');
            mqttStore.connected = null;
        }
    },

    init() {
        // Check immediately on init
        this.checkMqttStatus();

        // Check every 5 seconds
        this.checkInterval = setInterval(() => {
            this.checkMqttStatus();
        }, 5000);
    },

    destroy() {
        if (this.checkInterval) {
            clearInterval(this.checkInterval);
            this.checkInterval = null;
        }
    }
};
