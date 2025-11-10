import { fetchWithNotif } from './fetchHelper.js';

// Helper function to convert FormData to object
function formDataToObject(formData) {
    const obj = {};
    for (const [key, value] of formData.entries()) {
        obj[key] = value;
    }
    return obj;
}

export default {
    saveWifiConfig(formDataOrEvent) {
        const data = formDataOrEvent instanceof FormData
            ? formDataToObject(formDataOrEvent)
            : formDataOrEvent;
        fetchWithNotif('/wifi-settings', { body: data });
    },
    resetWifiConfig() {
        fetchWithNotif('/reset-wifi-settings');
    },
    saveNetworkConfig(formDataOrEvent) {
        const data = formDataOrEvent instanceof FormData
            ? formDataToObject(formDataOrEvent)
            : formDataOrEvent;
        fetchWithNotif('/network-settings', { body: data });
    },
    resetNetworkConfig() {
        fetchWithNotif('/reset-network-settings');
    },
    saveMqttConfig(formDataOrEvent) {
        const data = formDataOrEvent instanceof FormData
            ? formDataToObject(formDataOrEvent)
            : formDataOrEvent;
        console.log('Saving MQTT config:', data);
        fetchWithNotif('/mqtt-settings', { body: data });
    },
    resetMqttConfig() {
        fetchWithNotif('/reset-mqtt-settings');
    }
};
