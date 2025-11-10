import { fetchWithNotif } from './fetchHelper.js';

export default {
    saveWifiConfig($formData) {
        fetchWithNotif('/wifi-settings', { body: $formData });
    },
    resetWifiConfig() {
        fetchWithNotif('/reset-wifi-settings');
    },
    saveNetworkConfig($formData) {
        fetchWithNotif('/network-settings', { body: $formData });
    },
    resetNetworkConfig() {
        fetchWithNotif('/reset-network-settings');
    }
};
