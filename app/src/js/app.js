import Alpine from 'alpinejs';
import formData from 'alpinejs-form-data';
import color from './coponents/color.js';
import notifs from './coponents/notifs.js';
import websocket from './coponents/websocket.js';
import endpoints from './coponents/endpoint.js';
import { createLedController } from './coponents/ledController.js';

window.Alpine = Alpine;

Alpine.plugin(formData);

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

Alpine.store('mqttModal', {
    open: false,
    toggle() {
        this.open = !this.open;
    },
    exec() {
        Alpine.store('endpoints').resetMqttConfig();
        this.toggle();
    }
});

Alpine.store('settings', {
    open: false,
    toggle() {
        this.open = !this.open;
    }
});

Alpine.store('versionModal', {
    open: false,
    toggle() {
        this.open = !this.open;
    }
});

Alpine.data('lukeLedCtrl', () => createLedController('luke_strip', '#FF00AA'));
Alpine.data('vaderLedCtrl', () => createLedController('vader_strip', '#00EAFF'));

Alpine.start();
