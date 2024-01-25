export default {
    websocket: null,
    nbFailure: 0,
    state: false,
    init() {
        let gateway = `ws://${window.location.hostname}/ws`;
        console.log('Trying to open a WebSocket connection on ' + gateway + '...');
        this.websocket = new WebSocket(gateway);
        this.websocket.onopen    = this.onOpen;
        this.websocket.onclose   = this.onClose;
        this.websocket.onmessage = this.onMessage;
    },
    onOpen() {
        console.log('Connection opened');
        let wsStore = Alpine.store('websocket');
        wsStore.nbFailure = 0;
        wsStore.state = true;
    },
    onClose() {
        console.log('Connection closed');
        let wsStore = Alpine.store('websocket');
        wsStore.nbFailure++;
        wsStore.state = false;
        if (wsStore.nbFailure < 5) {
            wsStore.init();
        }
    },
    onMessage(wsevent) {
        let event = new CustomEvent('wschanged', {
            bubbles: true,
            cancelable: true,
            detail: wsevent.data,
        });
        window.dispatchEvent(event);
    }
}