const IMAGE_NAMES = ["abl"];

const state = {
  confirmStep: 0,
  confirmAction: "flash",
  needDuplicateWarn: false,
  moduleDir: "",
  scriptPath: "",
  status: null,
  pollTimer: null,
  prevStatusRaw: "",
};

const elements = {
  stateChip: document.getElementById("stateChip"),
  slotChip: document.getElementById("slotChip"),
  currentSlot: document.getElementById("currentSlot"),
  targetSlot: document.getElementById("targetSlot"),
  imageCount: document.getElementById("imageCount"),
  taskMessage: document.getElementById("taskMessage"),
  updatedAt: document.getElementById("updatedAt"),
  imageTableBody: document.getElementById("imageTableBody"),
  logOutput: document.getElementById("logOutput"),
  flashButton: document.getElementById("flashButton"),
  restoreButton: document.getElementById("restoreButton"),
  clearLogButton: document.getElementById("clearLogButton"),
  refreshButton: document.getElementById("refreshButton"),
  confirmModal: document.getElementById("confirmModal"),
  confirmText: document.getElementById("confirmText"),
  nextConfirmButton: document.getElementById("nextConfirmButton"),
  cancelConfirmButton: document.getElementById("cancelConfirmButton"),
  updateEfispCheckbox: document.getElementById("updateEfispCheckbox"),
};

function getKsuBridge() {
  return globalThis.ksu || window.ksu || null;
}

function shellQuote(value) {
  return `'${String(value).replace(/'/g, `'\\''`)}'`;
}

function toast(message) {
  getKsuBridge()?.toast?.(message);
}

function moduleInfo() {
  const bridge = getKsuBridge();
  if (!bridge?.moduleInfo) throw new Error("当前页面不在 KernelSU WebUI 环境中");
  const raw = bridge.moduleInfo();
  return typeof raw === "string" ? JSON.parse(raw) : raw;
}

function extractStdout(raw) {
  if (raw == null) return "";
  if (typeof raw === "string") {
    try {
      const obj = JSON.parse(raw);
      if (typeof obj?.stdout === "string") return obj.stdout;
      if (typeof obj?.out === "string") return obj.out;
    } catch {}
    return raw;
  }
  if (typeof raw?.stdout === "string") return raw.stdout;
  if (typeof raw?.out === "string") return raw.out;
  return String(raw);
}

function exec(command) {
  const bridge = getKsuBridge();
  if (!bridge?.exec) throw new Error("KernelSU exec API 不可用");
  return extractStdout(bridge.exec(command));
}

function runScript(action, arg) {
  const parts = [`MODDIR=${shellQuote(state.moduleDir)}`, "sh", shellQuote(state.scriptPath), action];
  if (arg) parts.push(shellQuote(arg));
  return exec(parts.join(" ")).replace(/\t/g, "\n");
}

function parseKeyValueOutput(output) {
  const info = {};
  for (const line of output.split(/\r?\n/)) {
    if (!line) continue;
    const eq = line.indexOf("=");
    if (eq > 0) info[line.slice(0, eq)] = line.slice(eq + 1);
  }
  return info;
}

function escapeHtml(str) {
  return str.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/"/g, "&quot;");
}

function parseBackupTimestamp(filePath) {
  if (!filePath) return null;
  const match = String(filePath).match(/_(\d{8})-(\d{6})_[^_]+_backup\.img$/);
  if (!match) return null;

  const datePart = match[1];
  const timePart = match[2];
  const year = Number(datePart.slice(0, 4));
  const month = Number(datePart.slice(4, 6));
  const day = Number(datePart.slice(6, 8));
  const hour = Number(timePart.slice(0, 2));
  const minute = Number(timePart.slice(2, 4));
  const second = Number(timePart.slice(4, 6));

  if ([year, month, day, hour, minute, second].some((v) => Number.isNaN(v))) {
    return null;
  }

  return new Date(year, month - 1, day, hour, minute, second);
}

function shouldWarnDuplicateFlash() {
  const status = state.status || {};
  if (status.HAS_BACKUP !== "1") return false;
  if (status.HAS_SAVED === "1") return false;

  const backupTime = parseBackupTimestamp(status.LATEST_BACKUP);
  if (!backupTime) return false;

  const diffMs = Math.abs(Date.now() - backupTime.getTime());
  return diffMs < 24 * 60 * 60 * 1000;
}

function renderTable(status, currentSlot, targetSlot) {
  if (currentSlot === "-" || targetSlot === "-") {
    elements.imageTableBody.innerHTML =
      '<tr><td colspan="4" class="empty-row">等待槽位检测</td></tr>';
    return;
  }

  elements.imageTableBody.innerHTML = IMAGE_NAMES.map((name) => {
    let srcPath = `/dev/block/by-name/${name}${currentSlot}`;
    let opText = "分区拷贝";
    let opClass = "status-pill ok";

    if (name === "abl" && status?.HAS_SAVED === "1" && status?.LATEST_SAVED) {
      srcPath = status.LATEST_SAVED;
      opText = "文件刷写(saved)";
      opClass = "status-pill";
    }

    const dstPath = `/dev/block/by-name/${name}${targetSlot}`;
    return `
      <tr>
        <td>${escapeHtml(name)}</td>
        <td class="caption">${escapeHtml(srcPath)}</td>
        <td>${escapeHtml(dstPath)}</td>
        <td><span class="${opClass}">${escapeHtml(opText)}</span></td>
      </tr>
    `;
  }).join("");
}

function renderStatus(status) {
  state.status = status;

  const currentSlot = status.CURRENT_SLOT || "-";
  const targetSlot = status.TARGET_SLOT || "-";
  const running = status.RUNNING === "1";
  const taskState = status.STATE || "idle";
  const taskMessage = status.MESSAGE || "等待操作";

  elements.currentSlot.textContent = currentSlot;
  elements.targetSlot.textContent = targetSlot;
  elements.imageCount.textContent = String(IMAGE_NAMES.length);
  elements.taskMessage.textContent = taskMessage;
  elements.updatedAt.textContent = status.UPDATED_AT || "-";

  elements.stateChip.textContent = running ? "任务运行中" : `状态: ${taskState}`;
  elements.stateChip.className = "chip";
  if (taskState === "success") {
    elements.stateChip.classList.add("chip-success");
  } else if (taskState === "error") {
    elements.stateChip.classList.add("chip-danger");
  } else if (taskState === "warning") {
    elements.stateChip.classList.add("chip-warn");
  } else if (running) {
    elements.stateChip.classList.add("chip-warn");
  }

  elements.slotChip.textContent =
    currentSlot !== "-" && targetSlot !== "-"
      ? `当前 ${currentSlot} → 目标 ${targetSlot}`
      : "槽位未知";

  elements.flashButton.disabled = running || currentSlot === "-" || targetSlot === "-";
  elements.restoreButton.disabled = running || currentSlot === "-" || status.HAS_BACKUP !== "1";
  elements.clearLogButton.disabled = running;

  renderTable(status, currentSlot, targetSlot);
}

function refreshStatus() {
  try {
    const raw = runScript("status");
    if (raw === state.prevStatusRaw) return state.status;
    state.prevStatusRaw = raw;
    const status = parseKeyValueOutput(raw);
    renderStatus(status);
    return status;
  } catch (error) {
    elements.stateChip.textContent = "状态读取失败";
    elements.stateChip.className = "chip chip-danger";
    elements.taskMessage.textContent = error.message;
    return null;
  }
}

function refreshLog() {
  try {
    const log = runScript("tail", "200").trim();
    elements.logOutput.textContent = log || "暂无日志输出";
    elements.logOutput.scrollTop = elements.logOutput.scrollHeight;
  } catch (error) {
    elements.logOutput.textContent = `日志读取失败: ${error.message}`;
  }
}

function closeConfirmModal() {
  state.confirmStep = 0;
  state.confirmAction = "flash";
  state.needDuplicateWarn = false;
  elements.confirmModal.classList.add("hidden");
  elements.confirmModal.setAttribute("aria-hidden", "true");
  elements.nextConfirmButton.textContent = "继续";
}

function openConfirmModal(action = "flash") {
  state.confirmAction = action;
  const targetSlot = state.status?.TARGET_SLOT || "?";
  const withEfisp = Boolean(elements.updateEfispCheckbox?.checked);
  state.needDuplicateWarn = action === "flash" && shouldWarnDuplicateFlash();

  if (state.needDuplicateWarn) {
    state.confirmStep = 0;
    elements.confirmText.textContent =
      "存在一天内的备份文件，请确认是否是重复刷写";
    elements.nextConfirmButton.textContent = "确认继续";
  } else {
    state.confirmStep = 1;
    elements.nextConfirmButton.textContent = "继续确认";
  }

  if (action === "restore") {
    elements.confirmText.textContent =
      "第一次确认: 将把 /data/adb/abl_backup 中最近的 abl 备份恢复到当前槽位，同时先保存当前槽位 abl。";
    elements.nextConfirmButton.textContent = "继续确认";
  } else {
    if (!state.needDuplicateWarn) {
      elements.confirmText.textContent = withEfisp
        ? `第一次确认: 将把当前槽位的 BL 分区拷贝到槽位 ${targetSlot}，并更新 efisp。请确认槽位无误。`
        : `第一次确认: 将把当前槽位的 BL 分区拷贝到槽位 ${targetSlot}，不更新 efisp。请确认槽位无误。`;
    }
  }

  elements.confirmModal.classList.remove("hidden");
  elements.confirmModal.setAttribute("aria-hidden", "false");
}

function handleConfirmProgress() {
  if (state.confirmAction === "flash" && state.needDuplicateWarn && state.confirmStep === 0) {
    const targetSlot = state.status?.TARGET_SLOT || "?";
    const withEfisp = Boolean(elements.updateEfispCheckbox?.checked);
    state.confirmStep = 1;
    elements.confirmText.textContent = withEfisp
      ? `第一次确认: 将把当前槽位的 BL 分区拷贝到槽位 ${targetSlot}，并更新 efisp。请确认槽位无误。`
      : `第一次确认: 将把当前槽位的 BL 分区拷贝到槽位 ${targetSlot}，不更新 efisp。请确认槽位无误。`;
    elements.nextConfirmButton.textContent = "继续确认";
    return;
  }

  if (state.confirmStep === 1) {
    state.confirmStep = 2;
    elements.confirmText.textContent = state.confirmAction === "restore"
      ? "第二次确认: 恢复将覆盖当前槽位 abl，错误镜像可能导致设备无法启动。确认后将立即开始恢复。"
      : "第二次确认: 这是高风险写入操作，错误操作可能导致目标槽位无法启动。确认后将立即开始刷写。";
    elements.nextConfirmButton.textContent = "确认刷写";
    return;
  }

  const action = state.confirmAction;
  closeConfirmModal();
  if (action === "restore") {
    startRestore();
  } else {
    startFlash();
  }
}

function startFlash() {
  const flashMode = elements.updateEfispCheckbox?.checked ? "update-efisp" : "skip-efisp";
  try {
    const output = parseKeyValueOutput(runScript("start", flashMode));
    if (output.ALREADY_RUNNING) {
      toast("已有刷写任务在运行");
    } else if (output.STARTED === "1") {
      toast("刷写任务已启动");
    } else if (output.FINISHED === "success") {
      toast("刷写已完成");
    } else if (output.FINISHED === "warning") {
      toast("BL 刷写完成，但 efisp 未更新");
    } else if (output.FINISHED === "error") {
      toast("刷写任务已结束（失败）");
    } else {
      toast("刷写任务启动失败");
    }
  } catch (error) {
    toast(`启动失败: ${error.message}`);
  }

  manualRefresh();
}

function clearLog() {
  try {
    const output = parseKeyValueOutput(runScript("clear-log"));
    if (output.BUSY === "1") {
      toast("任务运行中，暂时不能清空日志");
      return;
    }
    toast("日志已清空");
  } catch (error) {
    toast(`清空失败: ${error.message}`);
  }

  manualRefresh();
}

function startRestore() {
  try {
    const output = parseKeyValueOutput(runScript("start-restore"));
    if (output.ALREADY_RUNNING) {
      toast("已有任务在运行");
    } else if (output.STARTED === "1") {
      toast("恢复任务已启动");
    } else if (output.FINISHED === "success") {
      toast("恢复已完成");
    } else if (output.FINISHED === "warning") {
      toast("恢复完成（有警告）");
    } else if (output.FINISHED === "error") {
      toast("恢复任务已结束（失败）");
    } else {
      toast("恢复任务启动失败");
    }
  } catch (error) {
    toast(`恢复启动失败: ${error.message}`);
  }

  manualRefresh();
}

function poll() {
  const status = refreshStatus();
  if (status?.RUNNING === "1") refreshLog();
  schedulePoll(status?.RUNNING === "1" ? 3000 : 8000);
}

function schedulePoll(ms) {
  clearTimeout(state.pollTimer);
  state.pollTimer = setTimeout(poll, ms);
}

function manualRefresh() {
  clearTimeout(state.pollTimer);
  state.prevStatusRaw = "";
  refreshStatus();
  refreshLog();
  schedulePoll(state.status?.RUNNING === "1" ? 3000 : 8000);
}

function init() {
  try {
    const info = moduleInfo();
    state.moduleDir = info.moduleDir;
    state.scriptPath = `${state.moduleDir}/bin/bl_flasher.sh`;
  } catch (error) {
    elements.stateChip.textContent = "WebUI 初始化失败";
    elements.stateChip.className = "chip chip-danger";
    elements.taskMessage.textContent = error.message;
    elements.flashButton.disabled = true;
    elements.clearLogButton.disabled = true;
    return;
  }

  elements.refreshButton.addEventListener("click", manualRefresh);
  elements.flashButton.addEventListener("click", () => openConfirmModal("flash"));
  elements.restoreButton.addEventListener("click", () => openConfirmModal("restore"));
  elements.clearLogButton.addEventListener("click", clearLog);
  elements.cancelConfirmButton.addEventListener("click", closeConfirmModal);
  elements.nextConfirmButton.addEventListener("click", handleConfirmProgress);
  elements.confirmModal.addEventListener("click", (event) => {
    if (event.target === elements.confirmModal) {
      closeConfirmModal();
    }
  });

  refreshStatus();
  refreshLog();
  schedulePoll(state.status?.RUNNING === "1" ? 3000 : 8000);
}

init();
