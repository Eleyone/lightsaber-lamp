/**
 * Factory function for creating LED controller objects
 * Eliminates 95% duplication between Luke and Vader controllers
 */
export function createLedController(stripId, defaultColor) {
    return {
        id: stripId,
        state: false,
        color: defaultColor,
        brightness: 50,
        colorpickerIsOpen: false,

        handleWsChangeEvent(event) {
            const message = JSON.parse(event.detail);
            const keys = Object.keys(message);
            for (let i = 0; i < keys.length; i++) {
                const key = keys[i];
                if (key === this.id) {
                    const properties = Object.keys(message[key]);
                    for (let j = 0; j < properties.length; j++) {
                        const property = properties[j];
                        let newValue = message[key][property];
                        if (property === 'brightness') {
                            const brightnessValue = (newValue / 255) * 100;
                            newValue = brightnessValue.toFixed(0);
                        }
                        this[property] = newValue;
                    }
                }
            }
        },

        init() {
            this.$watch('state', (value, oldValue) => {
                if (value !== oldValue && Alpine.store('websocket').state) {
                    const websocketMsg = { [this.id]: { state: value } };
                    Alpine.store('websocket').websocket.send(JSON.stringify(websocketMsg));
                }
            });

            this.$watch('color', (value, oldValue) => {
                if (value !== oldValue && Alpine.store('websocket').state) {
                    const websocketMsg = { [this.id]: { color: value } };
                    Alpine.store('websocket').websocket.send(JSON.stringify(websocketMsg));
                }
            });

            this.$watch('brightness', (value, oldValue) => {
                if (value !== oldValue && Alpine.store('websocket').state) {
                    const brightnessValue = 255 * (value / 100);
                    const websocketMsg = { [this.id]: { brightness: brightnessValue.toFixed(0) } };
                    Alpine.store('websocket').websocket.send(JSON.stringify(websocketMsg));
                }
            });
        }
    };
}
