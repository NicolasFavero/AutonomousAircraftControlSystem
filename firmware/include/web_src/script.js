"use strict";

const $ = id => document.getElementById(id);

const content = $("content");
const menu = $("menu");
const menuButton = $("menuButton");
const overlay = $("overlay");

let currentPage = "";
let statusTimer = null;
let lastStatus = null;
/* ---------- HTTP helpers ---------- */

async function GET(url){
  const r = await fetch(url);
  if(!r.ok) throw new Error(r.status);
  return r;
}
const GET_JSON = async url => (await GET(url)).json();
const GET_TEXT = async url => (await GET(url)).text();

async function POST(url, data = null){
  const r = await fetch(url, { method:"POST", body:data });
  const json = await r.json();
  if(!r.ok) throw json;
  return json;
}
async function checkSd(){

    try{

        const json = await POST("/api/checksd");

        alertSuccess(json);

        await loadStatus();

    }
    catch(error){

        alertError(error);

    }
}
async function testLora(){

    try{

        const json = await POST("/api/checklora");

        alertSuccess(json);

        await loadStatus();

    }
    catch(error){

        alertError(error);

    }
}
/* ---------- DOM helpers ---------- */

function text(id, value){
  const e = $(id);
  if(e) e.textContent = value;
}

function valueField(id, v){
  const e = $(id);
  if(!e) return "";
  if(v === undefined) return e.value;
  e.value = v;
}

function checked(id, v){
  const e = $(id);
  if(!e) return false;
  if(v === undefined) return e.checked;
  e.checked = v;
}

function led(id, state){
  const e = $(id);
  if(!e) return;
  e.classList.toggle("ok", state);
  e.classList.toggle("error", !state);
}

// Para status que só fazem sentido depois de um teste manual (SD, LoRa):
// antes de testar, fica cinza (nem ok, nem erro) -- assim uma falha
// nunca-testada nao aparenta ser um bug ("tudo vermelho" no primeiro boot).
function ledTri(id, tested, ok){
  const e = $(id);
  if(!e) return;

  if(!tested){
    e.classList.remove("ok", "error");
    e.classList.add("untested");
    return;
  }

  e.classList.remove("untested");
  e.classList.toggle("ok", ok);
  e.classList.toggle("error", !ok);
}
function setGpsLed(status){

    const e = $("gpsLed");

    if(!e) return;

    e.classList.remove(
        "gps-red",
        "gps-orange",
        "gps-yellow",
        "gps-green"
    );

    switch(status){

        case 0:
            e.classList.add("gps-red");
            break;

        case 1:
            e.classList.add("gps-orange");
            break;

        case 2:
            e.classList.add("gps-yellow");
            break;

        case 3:
            e.classList.add("gps-green");
            break;
    }
}
function num(v, digits = 2){
  return Number(v ?? 0).toFixed(digits);
}

function alertError(error){
  console.error(error);
  customAlert("Erro de comunicação.");
}

function alertSuccess(json){
  if(json && json.message) toast(json.message);
}

let toastTimer = null;
function toast(message, duration = 2500){
  const box = $("toastBox");
  if(!box) return;

  box.innerHTML = "";

  const el = document.createElement("div");
  el.className = "toast";
  el.textContent = message;
  box.appendChild(el);

  // Força um reflow antes de adicionar "show" pra garantir que a
  // transição de opacidade/translateY toque, mesmo em toasts
  // seguidos.
  void el.offsetWidth;
  el.classList.add("show");

  if(toastTimer) clearTimeout(toastTimer);
  toastTimer = setTimeout(() => el.classList.remove("show"), duration);
}

/* ---------- Modal (substitui alert/confirm/prompt nativos) ---------- */
// alert/confirm/prompt do navegador sempre mostram algo como
// "192.168.4.1 diz:" antes da mensagem -- isso vem do proprio
// navegador, a pagina nao tem como mudar esse texto. A solucao e ter
// um dialogo proprio, com a cara do resto da interface, no lugar
// desses tres.
function showModal({ message, showInput = false, inputValue = "", confirmText = "OK", cancelText = null }){
    return new Promise(resolve => {
        const overlay = $("modalOverlay");
        const box = $("modalBox");

        if(!overlay || !box){
            // Sem modal na pagina (nao deveria acontecer): cai pro
            // dialogo nativo pra nao travar a acao do usuario.
            if(showInput) resolve(window.prompt(message, inputValue));
            else if(cancelText) resolve(window.confirm(message));
            else{ window.alert(message); resolve(true); }
            return;
        }

        const safeValue = String(inputValue).replace(/"/g, "&quot;");

        box.innerHTML = `
            <p>${message}</p>
            ${showInput ? `<input id="modalInput" type="text" value="${safeValue}">` : ""}
            <div class="modalActions">
                ${cancelText ? `<button class="secondary" id="modalCancel">${cancelText}</button>` : ""}
                <button class="primary" id="modalConfirm">${confirmText}</button>
            </div>
        `;

        overlay.classList.add("open");

        const input = $("modalInput");
        input?.focus();
        input?.select();

        function close(result){
            overlay.classList.remove("open");
            resolve(result);
        }

        $("modalConfirm").onclick =
            () => close(showInput ? (input?.value ?? "") : true);

        $("modalCancel")?.addEventListener(
            "click",
            () => close(showInput ? null : false)
        );
    });
}

function customAlert(message){
    return showModal({ message, confirmText: "OK" });
}

function customConfirm(message){
    return showModal({ message, confirmText: "Confirmar", cancelText: "Cancelar" });
}

function customPrompt(message, defaultValue = ""){
    return showModal({
        message,
        showInput: true,
        inputValue: defaultValue,
        confirmText: "OK",
        cancelText: "Cancelar",
    });
}

/* ---------- Menu ---------- */

function closeMenu(){
  menu.classList.remove("open");
  overlay.classList.remove("open");
}

menuButton.onclick = () => {
  menu.classList.toggle("open");
  overlay.classList.toggle("open");
};

overlay.onclick = closeMenu;

document.querySelectorAll("#menu button").forEach(button => {
  button.onclick = () => loadPage(button.dataset.page);
});

/* ---------- Page loading ---------- */

async function loadPage(page){
  if(currentPage === page){
    closeMenu();
    return;
  }

  stopStatus();
  stopConsole();

  try{
    content.innerHTML = await GET_TEXT("/page/" + page);
    currentPage = page;
    initializePage(page);
    closeMenu();
  }
  catch(error){
    alertError(error);
  }
}

function initializePage(page){
  switch(page){

    case "status":
      initializeLiveStatus();
      break;

    case "console":
      initializeConsole();
      break;

    case "logs":
      loadLogs();
      $("deleteAllLogs")?.addEventListener("click", deleteAllLogs);
      break;

    case "offsets":

    loadConfig();
    loadStatus();

    $("refreshImu")?.addEventListener(
        "click",
        loadStatus
    );

    $("zeroPitch")?.addEventListener(
        "click",
        () => tareAxis("pitch")
    );

    $("zeroRoll")?.addEventListener(
        "click",
        () => tareAxis("roll")
    );

    $("zeroYaw")?.addEventListener(
        "click",
        () => tareAxis("yaw")
    );

    $("saveOffsets")?.addEventListener(
        "click",
        async () => {
            await save("offset");

            // O offset novo so passa a valer depois do firmware
            // processar o evento (loop de ~100ms) -- espera um
            // pouco e atualiza a leitura exibida, pra proxima tara
            // ja partir de um valor fresco em vez do antigo.
            setTimeout(loadStatus, 300);
        }
    );

    break;

    case "pid":

    loadConfig();

    $("savePid")?.addEventListener(
        "click",
        () => save("pid")
    );

    break;

    case "system":

    loadConfig();

    $("saveSystem")?.addEventListener(
        "click",
        () => save("system")
    );

    break;

    case "lora":

    loadConfig();

    $("saveLora")?.addEventListener(
        "click",
        () => save("lora")
    );

    break;

    case "flight":

    loadStatus();
    startStatus();

    $("startFlight")?.addEventListener(
        "click",
        startFlight
    );

    $("endFlight")?.addEventListener(
        "click",
        endFlight
    );

    $("cancelCountdown")?.addEventListener(
        "click",
        cancelFlight
    );

    $("resetFlight")?.addEventListener(
        "click",
        resetFlight
    );

    $("restartESP")?.addEventListener(
        "click",
        restartESP
    );

    break;
    }
}
let tareBusy = false;
async function tareAxis(axis){
    // Busca um /api/status fresco na hora do clique: o offset
    // realmente ativo no firmware e a leitura sob esse mesmo
    // offset vem do MESMO pacote, entao o calculo nunca soma em
    // cima de um clique anterior nem ignora o offset atual --
    // ele sempre recalcula o angulo bruto do zero:
    // bruto = offsetAtivoNoFirmware + leituraAtualSobEsseOffset.
    //
    // tareBusy trava cliques repetidos enquanto uma tara ainda
    // esta em andamento (evita disparar varias buscas ao mesmo
    // tempo se clicar rapido demais).
    if(tareBusy) return;
    tareBusy = true;

    try{
        const data = await GET_JSON("/api/status");

        const offsetKey = axis + "Offset";
        const bruto = Number(data[offsetKey] ?? 0) + Number(data[axis] ?? 0);

        valueField(axis + "Offset", bruto.toFixed(2));

        fillStatus(data);
    }
    catch(error){
        alertError(error);
    }
    finally{
        tareBusy = false;
    }
}
/* ---------- Console ---------- */

let consoleTimer = null;

function initializeConsole(){

    loadConsole();

    const auto = $("autoStatus");

    if(auto){

        auto.onchange = () => {

            if(auto.checked)
                startConsole();
            else
                stopConsole();

        };

        if(auto.checked)
            startConsole();
    }

    $("refreshTelemetry")?.addEventListener(
        "click",
        loadConsole
    );
}

function startConsole(){

    stopConsole();

    consoleTimer = setInterval(
        loadConsole,
        500
    );
}

function stopConsole(){

    if(consoleTimer == null)
        return;

    clearInterval(consoleTimer);

    consoleTimer = null;
}

async function loadConsole(){

    try{

        const json = await GET_TEXT(
            "/api/telemetry"
        );

        text(
            "telemetryJson",
            json
        );

    }
    catch(error){

        text(
            "telemetryJson",
            "Erro de comunicação."
        );

    }
}
/* ---------- Live status (página status) ---------- */

function initializeLiveStatus(){
  loadStatus();

  const auto = $("autoStatus");
  if(auto){
    auto.onchange = () => (auto.checked ? startStatus() : stopStatus());
    if(auto.checked) startStatus();
  }

  $("refreshStatus")?.addEventListener("click", loadStatus);
  $("checkSd")?.addEventListener("click", checkSd);
  $("testLora")?.addEventListener("click", testLora);
}

function startStatus(){
  stopStatus();
  statusTimer = setInterval(loadStatus, 500);
}

function stopStatus(){
  if(statusTimer == null) return;
  clearInterval(statusTimer);
  statusTimer = null;
}

async function loadStatus(){
  try{
    fillStatus(await GET_JSON("/api/status"));
  }
  catch(error){
    console.error(error);
  }
}

const STATUS_TEXT_FIELDS = {
  pitch: d => num(d.pitch),
  roll: d => num(d.roll),
  yaw: d => num(d.yaw),
  latitude: d => num(d.lat, 7),
  longitude: d => num(d.lon, 7),
  gpsAltitude: d => num(d.gpsAlt),
  baroAltitude: d => num(d.baroAlt),
  battery: d => num(d.battery),
  satellites: d => d.gpsSat,
  telemetryPeriod: d => d.telemetryMs,
  batteryLimit: d => num(d.batteryLimit),
  wifiClients: d => d.wifiClients,
  pitchOffset: d => num(d.pitchOffset),
  rollOffset: d => num(d.rollOffset),
  yawOffset: d => num(d.yawOffset),
  flightMode: d => d.flightMode,
};

function fillStatus(data){
  for(const [id, fn] of Object.entries(STATUS_TEXT_FIELDS)){
    text(id, fn(data));
  }

  led("imuLed", data.imuOk);
  led("bmpLed", data.bmpOk);
  ledTri("loraLed", data.loraTested, data.loraOk);
  ledTri("sdLed", data.sdTested, data.sdOk);

  setGpsLed(data.gpsStatus);

  updateCountdownUi(data);
  updateFlightActiveUi(data.flightMode);
  updateFlightButtonsUi(data.flightMode);

  text("pitchNow", num(data.pitch));
  text("rollNow", num(data.roll));
  text("yawNow", num(data.yaw));
  lastStatus = data;
}

/* ---------- Configuration (offsets / system / lora) ---------- */

const CONFIG_FIELDS = {
  pitchOffset: "pitchOffset",
  rollOffset: "rollOffset",
  yawOffset: "yawOffset",

  pitchKp: "pitchKp",
  pitchKi: "pitchKi",
  pitchKd: "pitchKd",

  rollKp: "rollKp",
  rollKi: "rollKi",
  rollKd: "rollKd",

  yawKp: "yawKp",
  yawKi: "yawKi",
  yawKd: "yawKd",

  batteryLimit: "batteryLimit",

  telemetryPeriod: "telemetryMs",
  lowBatteryTelemetryPeriod: "lowBatteryTelemetryMs",

  frequency: "frequency",
  bandwidth: "bandwidth",
  spreadingFactor: "spreadingFactor",
  codingRate: "codingRate",
  power: "power",
  preambleLength: "preambleLength",
};

async function loadConfig(){
  try{
    fillConfig(await GET_JSON("/api/config"));
  }
  catch(error){
    alertError(error);
  }
}

function fillConfig(cfg){

    for(const [id, key] of Object.entries(CONFIG_FIELDS))
        valueField(id, cfg[key]);

    checked(
        "preFlightTelemetryEnabled",
        cfg.preFlightTelemetryEnabled
    );

    valueField(
        "syncWord",
        Number(cfg.syncWord).toString(16).toUpperCase()
    );
}

const SAVE_FORMS = {
  offset: {
    url: "/api/offsets",
    fields: { pitch:"pitchOffset", roll:"rollOffset", yaw:"yawOffset" },
  },
  pid: {
    url: "/api/pid",
    fields: {
      pitchKp:"pitchKp", pitchKi:"pitchKi", pitchKd:"pitchKd",
      rollKp:"rollKp", rollKi:"rollKi", rollKd:"rollKd",
      yawKp:"yawKp", yawKi:"yawKi", yawKd:"yawKd",
    },
  },
  system: {
      url: "/api/system",
      fields: {
          battery: "batteryLimit",
          telemetry: "telemetryPeriod",
          lowBatteryTelemetry: "lowBatteryTelemetryPeriod",
      },
      checks: {
          preFlightTelemetry: "preFlightTelemetryEnabled",
      },
  },
  lora: {
    url: "/api/lora",
    fields: {
      freq:"frequency", bw:"bandwidth", sf:"spreadingFactor",
      cr:"codingRate", power:"power", sync:"syncWord", pre:"preambleLength",
    },
  },
};
async function save(type){

    const form = SAVE_FORMS[type];
    const data = new URLSearchParams();

    for(const [param, id] of Object.entries(form.fields))
        data.append(param, valueField(id));

    if(form.checks){

        for(const [param, id] of Object.entries(form.checks))
            data.append(param, checked(id) ? 1 : 0);

    }

    try{

        alertSuccess(await POST(form.url, data));

        if(type === "offset"){

            // Antes o campo era forçado pra "0" aqui, o que fazia
            // "Offsets Atuais" mentir sobre o valor realmente salvo
            // a partir do primeiro save (so ficava correto de novo
            // depois de zerar na mao). Agora busca a config de
            // verdade no firmware -- o campo só mostra 0 quando o
            // offset salvo for mesmo 0 (ex: logo apos um flash novo).
            // Um pequeno atraso da tempo do firmware processar o
            // evento antes de reler.
            setTimeout(loadConfig, 300);

            await loadStatus();
        }

    }
    catch(error){

        alertError(error);

    }
}
/* ---------- Flight control ---------- */

// A contagem regressiva antes era um timer local de 10s, desacoplado
// do firmware -- isso dessincronizava fácil (navegar pra outra
// página e voltar durante a contagem, reativar o voo e iniciar de
// novo, etc), fazendo o card sumir cedo demais. Agora o numero vem
// pronto do firmware a cada /api/status (countdownRemainingMs), essa
// função só exibe o que chegou -- sem timer local, sem estado que
// possa ficar "preso" de uma contagem pra outra.
function updateCountdownUi(data){
    const card = $("countdownCard");

    if(!card)
        return;

    const inCountdown = data.flightMode === "COUNTDOWN";

    card.style.display = inCountdown ? "" : "none";

    if(!inCountdown)
        return;

    text("countdownFile", data.currentLog || "-");

    const numberEl = $("countdownNumber");
    const secondsLeft = Math.ceil((data.countdownRemainingMs ?? 0) / 1000);

    if(secondsLeft > 0){
        numberEl?.classList.remove("launch");
        text("countdownNumber", secondsLeft);
    }
    else{
        numberEl?.classList.add("launch");
        text("countdownNumber", "LANÇAR!");
    }
}

// Card separado, visivel so durante o FLIGHT de verdade -- o
// countdownCard some assim que o modo deixa de ser COUNTDOWN
// (inclusive quando vira FLIGHT), entao esse card substitui ele
// pra deixar claro que o voo esta em andamento.
function updateFlightActiveUi(mode){
    const card = $("flightActiveCard");

    if(!card)
        return;

    card.style.display = (mode === "FLIGHT") ? "" : "none";
}

// Iniciar Voo so faz sentido em CONFIG (e o unico estado em que
// /api/start e aceito). Finalizar Voo (na aba Comandos) so aparece
// durante o FLIGHT -- durante o COUNTDOWN quem cancela e o botao
// dedicado dentro do proprio card da contagem regressiva.
function updateFlightButtonsUi(mode){
    const startBtn = $("startFlight");
    const endBtn = $("endFlight");
    const resetBtn = $("resetFlight");

    if(!startBtn || !endBtn)
        return;

    const canStart = mode === "CONFIG";
    const canEnd = mode === "FLIGHT";
    const canReset = mode === "LANDED";

    startBtn.style.display = canStart ? "" : "none";
    endBtn.style.display = canEnd ? "" : "none";

    if(resetBtn)
        resetBtn.style.display = canReset ? "" : "none";
}

async function startFlight(){

    const problems = [];

    if(lastStatus){

        if(!lastStatus.imuOk)
            problems.push("• IMU");

        if(!lastStatus.bmpOk)
            problems.push("• BMP280");

        if(!lastStatus.loraOk)
            problems.push(lastStatus.loraTested ? "• LoRa (falhou no teste)" : "• LoRa (ainda não testado)");

        if(!lastStatus.sdOk)
            problems.push(lastStatus.sdTested ? "• Cartão SD (falhou no teste)" : "• Cartão SD (ainda não testado)");

        switch(lastStatus.gpsStatus){

            case 0:
                problems.push("• GPS sem comunicação");
                break;

            case 1:
                problems.push("• GPS sem FIX");
                break;

            case 2:
                problems.push("• GPS com FIX insuficiente");
                break;
        }
    }

    if(problems.length){

        const msg =
            "Os seguintes módulos apresentam problemas:\n\n" +
            problems.join("\n") +
            "\n\nDeseja continuar mesmo assim?";

        if(!(await customConfirm(msg)))
            return;
    }

    if(!(await customConfirm("Iniciar voo? Tem certeza?")))
        return;

    try{
        alertSuccess(await POST("/api/start"));
        await loadStatus();
    }
    catch(error){
        alertError(error);
    }
}

async function endFlight(){
    if(!(await customConfirm(
        "Finalizar voo?\n\nO modo passará para LANDED."
    )))
        return;

    // O backend fecha o arquivo do log ao processar /api/end, entao
    // currentLog ja vem vazio na resposta/proxima leitura -- captura
    // o nome AGORA, antes de mandar o POST.
    const fileName = lastStatus?.currentLog || "";

    try{
        await POST("/api/end");
        toast(fileName ? `Voo finalizado em ${fileName}.` : "Voo finalizado.");
        await loadStatus();
    }
    catch(error){
        alertError(error);
    }
}

async function cancelFlight(){
    if(!(await customConfirm("Cancelar o voo?")))
        return;

    const fileName = lastStatus?.currentLog || "";

    try{
        await POST("/api/end");
        toast(fileName ? `Voo cancelado em ${fileName}.` : "Voo cancelado.");
        await loadStatus();
    }
    catch(error){
        alertError(error);
    }
}

async function resetFlight(){
    if(!(await customConfirm("Reativar o voo? Isso libera um novo Iniciar Voo sem reiniciar o ESP32.")))
        return;

    try{
        alertSuccess(await POST("/api/reset"));
        await loadStatus();
    }
    catch(error){
        alertError(error);
    }
}

async function restartESP(){
  if(!(await customConfirm("Reiniciar ESP32?"))) return;

  try{
    alertSuccess(await POST("/api/restart"));
  }
  catch(error){
    alertError(error);
  }
}
/* ---------- Logs ---------- */

async function loadLogs(){

    try{

        // /api/logs devolve um array JSON puro (ex: ["voo1.csv"]),
        // nao um objeto {files:[...]}. Usar json.files aqui sempre
        // dava undefined e quebrava fillLogs() antes de preencher
        // a tabela -- por isso a pagina nunca listava nada.
        const files = await GET_JSON("/api/logs");

        // /api/status traz o nome do log em andamento (currentLog,
        // vazio se nenhum voo estiver gravando agora). Se essa
        // chamada falhar por qualquer motivo, a lista de logs ainda
        // e mostrada normalmente, so sem a marcação de "Em andamento".
        let currentLog = "";

        try{
            const status = await GET_JSON("/api/status");
            currentLog = status.currentLog || "";
        }
        catch(error){
            console.error(error);
        }

        fillLogs(files, currentLog);

    }
    catch(error){

        alertError(error);

    }
}

function fillLogs(files, currentLog = ""){

    files = files || [];

    const table = $("logsTable");

    if(!table)
        return;

    // O ESP32 nao tem RTC, entao os arquivos em si nao guardam data
    // real -- isso aqui e so o horario do navegador de quem esta
    // olhando a pagina agora, pra servir de referencia. Antes ficava
    // como uma unica linha de "ultima atualizacao" embaixo da tabela;
    // agora aparece embaixo de cada arquivo, igual ao resto da lista.
    const referenceDate = new Date().toLocaleString("pt-BR");

    if(!files.length){

        table.innerHTML = `
            <tr>
                <td colspan="2">
                    Nenhum log encontrado.
                </td>
            </tr>
        `;

        return;
    }

    table.innerHTML = "";

    for(const file of files){

        const inProgress = currentLog && file === currentLog;

        table.insertAdjacentHTML(
            "beforeend",

            `
            <tr>

                <td>
                    ${file}
                    ${inProgress ? '<br><span class="tag tag-warn">Em andamento</span>' : ""}
                    <br><span class="logDate">${referenceDate}</span>
                </td>

                <td>

                    <button class="secondary" onclick="downloadLog('${file}')">
                        Baixar
                    </button>

                    <button class="secondary" onclick="renameLog('${file}')">
                        Renomear
                    </button>

                    <button
                        class="danger"
                        onclick="deleteLog('${file}')">

                        Excluir

                    </button>

                </td>

            </tr>
            `
        );
    }
}

function downloadLog(file){

    window.location =
        "/api/logs/download?file=" +
        encodeURIComponent(file);

}

async function renameLog(file){

    let newName = await customPrompt(
        "Novo nome para \"" + file + "\":",
        file
    );

    if(newName === null)
        return;

    newName = newName.trim();

    if(!newName || newName === file)
        return;

    if(!newName.toLowerCase().endsWith(".csv"))
        newName += ".csv";

    const data = new URLSearchParams();

    data.append("file", file);
    data.append("newName", newName);

    try{

        alertSuccess(
            await POST(
                "/api/logs/rename",
                data
            )
        );

        loadLogs();

    }
    catch(error){

        alertError(error);

    }
}

async function deleteLog(file){

    if(!(await customConfirm(
        "Excluir " + file + " ?"
    )))
        return;

    const data = new URLSearchParams();

    data.append(
        "file",
        file
    );

    try{

        alertSuccess(
            await POST(
                "/api/logs/delete",
                data
            )
        );

        loadLogs();

    }
    catch(error){

        alertError(error);

    }
}
async function deleteAllLogs(){

    if(!(await customConfirm(
        "Excluir TODOS os logs?"
    )))
        return;

    try{

        alertSuccess(
            await POST(
                "/api/logs/deleteAll"
            )
        );

        loadLogs();

    }
    catch(error){

        alertError(error);

    }
}

/* ---------- Boot ---------- */

window.addEventListener("load", () => loadPage("status"));
