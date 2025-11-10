export default {
    websocket: null,
    nbFailure: 0,
    state: false,
    init() {
        const gateway = `ws://${window.location.hostname}/ws`;
        console.warn('Trying to open a WebSocket connection on ' + gateway + '...');
        this.websocket = new WebSocket(gateway);
        this.websocket.onopen = this.onOpen;
        this.websocket.onclose = this.onClose;
        this.websocket.onmessage = this.onMessage;
    },
    onOpen() {
        console.warn('Connection opened');
        const wsStore = Alpine.store('websocket');
        wsStore.nbFailure = 0;
        wsStore.state = true;
    },
    onClose() {
        console.warn('Connection closed');
        const wsStore = Alpine.store('websocket');
        wsStore.nbFailure++;
        wsStore.state = false;
        if (wsStore.nbFailure < 5) {
            wsStore.init();
        }
    },
    onMessage(wsevent) {
        const event = new CustomEvent('wschanged', {
            bubbles: true,
            cancelable: true,
            detail: wsevent.data
        });
        window.dispatchEvent(event);
    }
};
