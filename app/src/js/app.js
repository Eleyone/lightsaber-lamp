import Alpine from 'alpinejs'
import formData from 'alpinejs-form-data'
import color from './coponents/color.js'
import notifs from './coponents/notifs.js'
import websocket from './coponents/websocket.js'
import endpoints from './coponents/endpoint.js'

window.Alpine = Alpine;

Alpine.plugin(formData)

Alpine.store('colors', color);

Alpine.store('notifs', notifs);

Alpine.store('websocket', websocket);

Alpine.store('endpoints', endpoints);

Alpine.store('wifiModal', {
    open: false,
    toggle() {
        this.open = !this.open;
    },
    exec() {
        Alpine.store('endpoints').resetWifiConfig();
        this.toggle();
    }
});

Alpine.store('networkModal', {
    open: false,
    toggle() {
        this.open = !this.open;
    },
    exec() {
        Alpine.store('endpoints').resetNetworkConfig();
        this.toggle();
    }
});

Alpine.store('settings', {
    open: false,
    toggle() {
        this.open = !this.open;
    },
});

Alpine.data('lukeLedCtrl', () => ({
    id: 'luke_strip',
    state: false,
    color: "#FF00AA",
    brightness: 50,
    colorpickerIsOpen: false,
    handleWsChangeEvent(event) {
        let message = JSON.parse(event.detail);
        let keys = Object.keys(message);
        for (let i = 0; i < keys.length; i++){
            let key = keys[i];
            if (key === this.id) {
                let properties = Object.keys(message[key]);
                for (let j = 0; j < properties.length; j++){
                    let property = properties[j];
                    let newValue = message[key][property];
                    if (property == 'brightness') {
                        let brightnessValue = (newValue/255)*100;
                        newValue = brightnessValue.toFixed(0);
                    }
                    this[property] = newValue;
                }
            }
        }
    },
    init() {
        this.$watch('state', (value, oldValue) => {
            if (value != oldValue && Alpine.store('websocket').state) {
                let websocketMsg = {
                    "luke_strip": {
                        "state": value
                    }
                };
                let msg = JSON.stringify(websocketMsg);
                Alpine.store('websocket').websocket.send(msg);
            }
        });
        this.$watch('color', (value, oldValue) => {
            if (value != oldValue && Alpine.store('websocket').state) {
                let websocketMsg = {
                    "luke_strip": {
                        "color": value
                    }
                };
                Alpine.store('websocket').websocket.send(JSON.stringify(websocketMsg));
            }
        });
        this.$watch('brightness', (value, oldValue) => {
            if (value != oldValue && Alpine.store('websocket').state) {
                let brightnessValue = 255*(value/100);
                let websocketMsg = {
                    "luke_strip": {
                        "brightness": brightnessValue.toFixed(0)
                    }
                };
                Alpine.store('websocket').websocket.send(JSON.stringify(websocketMsg));
            }
        });
    }
}));
Alpine.data('vaderLedCtrl', () => ({
    id: 'vader_strip',
    state: false,
    color: "#00EAFF",
    brightness: 50,
    colorpickerIsOpen: false,
    handleWsChangeEvent(event) {
        let message = JSON.parse(event.detail);
        let keys = Object.keys(message);
        for (let i = 0; i < keys.length; i++){
            let key = keys[i];
            if (key === this.id) {
                let properties = Object.keys(message[key]);
                for (let j = 0; j < properties.length; j++){
                    let property = properties[j];
                    let newValue = message[key][property];
                    if (property == 'brightness') {
                        let brightnessValue = (newValue/255)*100;
                        newValue = brightnessValue.toFixed(0);
                    }
                    this[property] = newValue;
                }
            }
        }
    },
    init() {
        this.$watch('state', (value, oldValue) => {
            if (value != oldValue && Alpine.store('websocket').state) {
                let websocketMsg = {
                    "vader_strip": {
                        "state": value
                    }
                };
                Alpine.store('websocket').websocket.send(JSON.stringify(websocketMsg));
            }
        });
        this.$watch('color', (value, oldValue) => {
            if (value != oldValue && Alpine.store('websocket').state) {
                let websocketMsg = {
                    "vader_strip": {
                        "color": value
                    }
                };
                Alpine.store('websocket').websocket.send(JSON.stringify(websocketMsg));
            }
        });
        this.$watch('brightness', (value, oldValue) => {
            if (value != oldValue && Alpine.store('websocket').state) {
                let brightnessValue = 255*(value/100);
                let websocketMsg = {
                    "vader_strip": {
                        "brightness": brightnessValue.toFixed(0)
                    }
                };
                Alpine.store('websocket').websocket.send(JSON.stringify(websocketMsg));
            }
        });
    }
}));

Alpine.start();