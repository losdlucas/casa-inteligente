// ==========================================
// SMART HOME
// Controle da interface
// ==========================================


// ==========================================
// ESTADO DO SISTEMA
// ==========================================

const houseState = {

    lights: {

        sala: false,
        cozinha: false,
        quarto: false,
        banheiro: false,
        externa: false

    },

    alarm: false,

    motion: false

};


// ==========================================
// ELEMENTOS
// ==========================================

const lightsCounter =
    document.getElementById("lightsCounter");

const alarmButton =
    document.getElementById("alarmButton");

const alarmStatus =
    document.getElementById("alarmStatus");

const alarmText =
    document.getElementById("alarmText");

const alarmDot =
    document.getElementById("alarmDot");

const progressBar =
    document.getElementById("progressBar");

const eventsList =
    document.getElementById("eventsList");


// ==========================================
// CONTROLE DAS LUZES
// ==========================================

async function toggleLight(room) {

    const novoEstado = !houseState.lights[room];

    try {

        const resposta = await fetch("/api/luz", {
            method: "POST",
            headers: {
                "Content-Type": "application/json"
            },
            body: JSON.stringify({
                comodo: room,
                estado: novoEstado
            })
        });

        if (!resposta.ok) {
            throw new Error("Erro ao enviar comando para o Flask.");
        }

        const dados = await resposta.json();

        houseState.lights[room] = dados.estado;

        updateLightInterface(room);
        updateLightsCounter();

        addEvent(
            houseState.lights[room]
                ? "💡"
                : "🌑",
            houseState.lights[room]
                ? `Luz da ${getRoomName(room)} ligada`
                : `Luz da ${getRoomName(room)} desligada`
        );

    } catch (erro) {

        console.error(erro);

        alert(
            "Não foi possível comunicar com o servidor Flask."
        );
    }
}


// ==========================================
// ATUALIZA VISUAL DA LUZ
// ==========================================

function updateLightInterface(room) {

    const card =
        document.querySelector(
            `[data-room="${room}"]`
        );


    const status =
        card.querySelector(".light-status");


    const buttonText =
        card.querySelector(".button-text");


    const isOn =
        houseState.lights[room];


    if (isOn) {

        card.classList.add("active");

        status.classList.remove("off");

        status.classList.add("on");

        status.textContent = "ON";

        buttonText.textContent =
            "Desligar luz";

    }

    else {

        card.classList.remove("active");

        status.classList.remove("on");

        status.classList.add("off");

        status.textContent = "OFF";

        buttonText.textContent =
            "Ligar luz";

    }

}


// ==========================================
// CONTADOR DE LUZES
// ==========================================

function updateLightsCounter() {

    const total =
        Object.values(houseState.lights)
            .filter(Boolean)
            .length;


    lightsCounter.textContent =
        `${total} ${total === 1 ? "ligada" : "ligadas"}`;

}


// ==========================================
// NOMES DOS CÔMODOS
// ==========================================

function getRoomName(room) {

    const names = {

        sala: "Sala",

        cozinha: "Cozinha",

        quarto: "Quarto",

        banheiro: "Banheiro",

        externa: "Área externa"

    };


    return names[room] || room;

}


// ==========================================
// ALARME
// ==========================================

let holdTimer = null;

let holdStart = null;

const HOLD_TIME = 2000;


// ==========================================
// INICIAR PRESSÃO
// ==========================================

function startAlarmHold() {

    // Se já estiver ligado,
    // não precisa segurar novamente.

    if (houseState.alarm) {

        return;

    }


    holdStart = Date.now();


    progressBar.style.width = "0%";


    alarmButton.classList.add("holding");


    holdTimer =
        setInterval(updateAlarmProgress, 20);

}


// ==========================================
// ATUALIZAR BARRA
// ==========================================

function updateAlarmProgress() {

    if (!holdStart) {

        return;

    }


    const elapsed =
        Date.now() - holdStart;


    const progress =
        Math.min(
            (elapsed / HOLD_TIME) * 100,
            100
        );


    progressBar.style.width =
        `${progress}%`;


    if (elapsed >= HOLD_TIME) {

        activateAlarm();

        stopAlarmHold();

    }

}


// ==========================================
// PARAR PRESSÃO
// ==========================================

function stopAlarmHold() {

    clearInterval(holdTimer);

    holdTimer = null;

    holdStart = null;


    if (!houseState.alarm) {

        progressBar.style.width = "0%";

    }


    alarmButton.classList.remove("holding");

}


// ==========================================
// ATIVAR / DESATIVAR ALARME
// ==========================================

function activateAlarm() {

    houseState.alarm = true;


    alarmButton.classList.add("active");

    alarmButton.innerHTML = `
        <span class="alarm-button-icon">
            🔓
        </span>

        <span>
            TOQUE PARA DESATIVAR
        </span>
    `;


    alarmStatus.textContent =
        "Ativado";


    alarmStatus.style.color =
        "#ff3b4d";


    alarmText.textContent =
        "Sistema de alarme ativo";


    alarmDot.classList.add("active");


    addEvent(
        "🚨",
        "Sistema de alarme ativado"
    );


    /*
    ==========================================
    FUTURO MQTT / API
    ==========================================

    Aqui o integrante pode enviar:

    MQTT:
    casa/alarme
    mensagem:
    "ON"

    OU API:

    fetch("/api/alarme", {
        method: "POST",
        headers: {
            "Content-Type": "application/json"
        },
        body: JSON.stringify({
            estado: true
        })
    });

    ==========================================
    */

}


// ==========================================
// DESATIVAR ALARME
// ==========================================

function deactivateAlarm() {

    houseState.alarm = false;


    alarmButton.classList.remove("active");


    alarmButton.innerHTML = `
        <span class="alarm-button-icon">
            🔒
        </span>

        <span>
            SEGURE PARA ATIVAR
        </span>
    `;


    alarmStatus.textContent =
        "Desativado";


    alarmStatus.style.color =
        "";


    alarmText.textContent =
        "Sistema desativado";


    alarmDot.classList.remove("active");


    addEvent(
        "🔓",
        "Sistema de alarme desativado"
    );


    /*
    ==========================================
    FUTURO MQTT / API
    ==========================================

    Enviar:

    casa/alarme
    "OFF"

    ==========================================
    */

}


// ==========================================
// EVENTOS DO BOTÃO DO ALARME
// ==========================================


// Mouse

alarmButton.addEventListener(
    "mousedown",
    startAlarmHold
);


alarmButton.addEventListener(
    "mouseup",
    handleAlarmRelease
);


alarmButton.addEventListener(
    "mouseleave",
    handleAlarmRelease
);


// Celular / Touch

alarmButton.addEventListener(
    "touchstart",
    function (event) {

        event.preventDefault();

        startAlarmHold();

    }
);


alarmButton.addEventListener(
    "touchend",
    function (event) {

        event.preventDefault();

        handleAlarmRelease();

    }
);


// ==========================================
// SOLTAR BOTÃO
// ==========================================

function handleAlarmRelease() {

    if (houseState.alarm) {

        deactivateAlarm();

    }

    else {

        stopAlarmHold();

    }

}


// ==========================================
// MOVIMENTO
// ==========================================

function detectMotion() {

    houseState.motion = true;


    const motionStatus =
        document.getElementById(
            "motionStatus"
        );


    const motionDescription =
        document.getElementById(
            "motionDescription"
        );


    const indicator =
        document.getElementById(
            "motionIndicator"
        );


    motionStatus.textContent =
        "Movimento detectado!";


    motionDescription.textContent =
        "Pessoa detectada na área externa";


    indicator.classList.add(
        "detected"
    );


    addEvent(
        "👤",
        "Movimento detectado na área externa"
    );


    // Liga a luz externa

    if (!houseState.lights.externa) {

        houseState.lights.externa = true;

        updateLightInterface("externa");

        updateLightsCounter();

        addEvent(
            "💡",
            "Luz externa ligada automaticamente"
        );

    }


    /*
    ==========================================
    FUTURO MQTT
    ==========================================

    Quando o ESP32 detectar movimento,
    o MQTT chamará esta função.

    Exemplo:

    MQTT
       ↓
    movimento
       ↓
    detectMotion()

    ==========================================
    */


    setTimeout(clearMotion, 5000);

}


// ==========================================
// LIMPAR MOVIMENTO
// ==========================================

function clearMotion() {

    houseState.motion = false;


    const motionStatus =
        document.getElementById(
            "motionStatus"
        );


    const motionDescription =
        document.getElementById(
            "motionDescription"
        );


    const indicator =
        document.getElementById(
            "motionIndicator"
        );


    motionStatus.textContent =
        "Nenhum movimento";


    motionDescription.textContent =
        "Área externa monitorada";


    indicator.classList.remove(
        "detected"
    );

}


// ==========================================
// ADICIONAR EVENTO
// ==========================================

function addEvent(icon, message) {

    const empty =
        eventsList.querySelector(
            ".empty-events"
        );


    if (empty) {

        empty.remove();

    }


    const event =
        document.createElement("div");


    event.className = "event";


    const time =
        new Date().toLocaleTimeString(
            "pt-BR",
            {
                hour: "2-digit",
                minute: "2-digit"
            }
        );


    event.innerHTML = `

        <div class="event-icon">
            ${icon}
        </div>

        <div class="event-info">

            <strong>
                ${message}
            </strong>

            <small>
                Sistema Smart Home
            </small>

        </div>

        <span class="event-time">
            ${time}
        </span>

    `;


    eventsList.prepend(event);


    // Limita a 5 eventos

    const events =
        eventsList.querySelectorAll(
            ".event"
        );


    if (events.length > 5) {

        events[events.length - 1].remove();

    }

}


// ==========================================
// INICIALIZAÇÃO
// ==========================================

updateLightsCounter();


// ==========================================
// TESTE DO SENSOR
// ==========================================

// Esta função é somente para testar
// enquanto o ESP32 ainda não estiver conectado.
//
// No console do navegador:
//
// detectMotion()
//
// Depois o integrante poderá remover
// esta parte quando colocar o MQTT.

console.log(
    "🏠 Smart Home iniciado!"
);

console.log(
    "Para testar o sensor, execute: detectMotion()"
);