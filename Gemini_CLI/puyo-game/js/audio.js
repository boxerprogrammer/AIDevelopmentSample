/**
 * js/audio.js
 * Web Audio API によるプログラミングBGM・効果音合成エンジン
 * 外部音声アセットを使用せず、純粋なオシレーターとノイズ生成で完全再現
 */

class SoundEngine {
  constructor() {
    this.ctx = null;
    this.isMuted = false;
    this.bgmVolume = 0.28;
    this.seVolume = 0.45;
    
    // オーディオノード
    this.masterGain = null;
    this.bgmGain = null;
    this.seGain = null;

    // BGM再生状態
    this.isPlayingBGM = false;
    this.bgmBpm = 136;
    this.currentStep = 0;
    this.nextNoteTime = 0;
    this.bgmTimerId = null;
    this.currentStage = 1;

    // ノイズバッファ（ドラム・おじゃま落下用）
    this.noiseBuffer = null;
  }

  /** 初期化（ユーザーインタラクション時に呼び出し） */
  init() {
    if (this.ctx) return;

    const AudioContextClass = window.AudioContext || window.webkitAudioContext;
    if (!AudioContextClass) {
      console.warn("Web Audio API is not supported in this browser.");
      return;
    }

    this.ctx = new AudioContextClass();

    // マスターゲイン
    this.masterGain = this.ctx.createGain();
    this.masterGain.gain.value = 1.0;
    this.masterGain.connect(this.ctx.destination);

    // BGMゲイン
    this.bgmGain = this.ctx.createGain();
    this.bgmGain.gain.value = this.bgmVolume;
    this.bgmGain.connect(this.masterGain);

    // SEゲイン
    this.seGain = this.ctx.createGain();
    this.seGain.gain.value = this.seVolume;
    this.seGain.connect(this.masterGain);

    // ノイズバッファ作成 (1秒分)
    this.createNoiseBuffer();
  }

  /** ユーザー操作時のResume */
  resume() {
    this.init();
    if (this.ctx && this.ctx.state === 'suspended') {
      this.ctx.resume();
    }
  }

  /** ミュート切り替え */
  toggleMute() {
    this.resume();
    this.isMuted = !this.isMuted;
    if (this.masterGain) {
      this.masterGain.gain.setValueAtTime(this.isMuted ? 0 : 1.0, this.ctx.currentTime);
    }
    return !this.isMuted;
  }

  /** ホワイトノイズバッファ生成 */
  createNoiseBuffer() {
    if (!this.ctx) return;
    const bufferSize = this.ctx.sampleRate * 2;
    const buffer = this.ctx.createBuffer(1, bufferSize, this.ctx.sampleRate);
    const data = buffer.getChannelData(0);
    for (let i = 0; i < bufferSize; i++) {
      data[i] = Math.random() * 2 - 1;
    }
    this.noiseBuffer = buffer;
  }

  // =========================================================================
  // SE (効果音) 合成関数群
  // =========================================================================

  /** 移動音（コツッ） */
  playMove() {
    this.resume();
    if (!this.ctx || this.isMuted) return;

    const t = this.ctx.currentTime;
    const osc = this.ctx.createOscillator();
    const gain = this.ctx.createGain();

    osc.type = 'triangle';
    osc.frequency.setValueAtTime(440, t);
    osc.frequency.exponentialRampToValueAtTime(180, t + 0.05);

    gain.gain.setValueAtTime(0.2, t);
    gain.gain.exponentialRampToValueAtTime(0.001, t + 0.05);

    osc.connect(gain);
    gain.connect(this.seGain);

    osc.start(t);
    osc.stop(t + 0.06);
  }

  /** 回転音（ピピッ） */
  playRotate() {
    this.resume();
    if (!this.ctx || this.isMuted) return;

    const t = this.ctx.currentTime;
    const osc = this.ctx.createOscillator();
    const gain = this.ctx.createGain();

    osc.type = 'square';
    osc.frequency.setValueAtTime(587.33, t); // D5
    osc.frequency.setValueAtTime(880, t + 0.03); // A5

    gain.gain.setValueAtTime(0.22, t);
    gain.gain.exponentialRampToValueAtTime(0.001, t + 0.08);

    osc.connect(gain);
    gain.connect(this.seGain);

    osc.start(t);
    osc.stop(t + 0.09);
  }

  /** 設置音（ポヨン） */
  playLand() {
    this.resume();
    if (!this.ctx || this.isMuted) return;

    const t = this.ctx.currentTime;
    const osc = this.ctx.createOscillator();
    const gain = this.ctx.createGain();

    osc.type = 'sine';
    osc.frequency.setValueAtTime(260, t);
    osc.frequency.exponentialRampToValueAtTime(120, t + 0.12);

    gain.gain.setValueAtTime(0.35, t);
    gain.gain.exponentialRampToValueAtTime(0.001, t + 0.12);

    osc.connect(gain);
    gain.connect(this.seGain);

    osc.start(t);
    osc.stop(t + 0.13);
  }

  /** ちぎり落下音 */
  playSplitDrop() {
    this.resume();
    if (!this.ctx || this.isMuted) return;

    const t = this.ctx.currentTime;
    const osc = this.ctx.createOscillator();
    const gain = this.ctx.createGain();

    osc.type = 'triangle';
    osc.frequency.setValueAtTime(520, t);
    osc.frequency.exponentialRampToValueAtTime(220, t + 0.09);

    gain.gain.setValueAtTime(0.2, t);
    gain.gain.exponentialRampToValueAtTime(0.001, t + 0.09);

    osc.connect(gain);
    gain.connect(this.seGain);

    osc.start(t);
    osc.stop(t + 0.1);
  }

  /**
   * 連鎖ボイス代替SE
   * 1連鎖, 2連鎖, 3連鎖...と音階と倍音が階段状に豪華に上昇！
   * @param {number} chainCount 1〜
   */
  playChain(chainCount) {
    this.resume();
    if (!this.ctx || this.isMuted) return;

    const t = this.ctx.currentTime;
    // 連鎖音階テーブル (Cメジャーペンタトニック/スケールアップ)
    const baseFreqs = [
      261.63, // 1: C4
      329.63, // 2: E4
      392.00, // 3: G4
      523.25, // 4: C5
      587.33, // 5: D5
      659.25, // 6: E5
      783.99, // 7: G5
      880.00, // 8: A5
      1046.50,// 9: C6
      1174.66,// 10: D6
      1318.51,// 11: E6
      1567.98 // 12+: G6
    ];

    const idx = Math.min(chainCount - 1, baseFreqs.length - 1);
    const freq = baseFreqs[Math.max(0, idx)];

    // 主音 (キラキラしたFM/シンセベル風)
    const osc1 = this.ctx.createOscillator();
    const osc2 = this.ctx.createOscillator();
    const gain = this.ctx.createGain();

    osc1.type = 'triangle';
    osc1.frequency.setValueAtTime(freq, t);

    // 5度上またはオクターブ上のハーモニー
    osc2.type = 'sine';
    osc2.frequency.setValueAtTime(freq * 1.5, t);
    osc2.frequency.exponentialRampToValueAtTime(freq * 2.0, t + 0.35);

    const volume = Math.min(0.4 + chainCount * 0.04, 0.7);
    gain.gain.setValueAtTime(volume, t);
    gain.gain.exponentialRampToValueAtTime(0.001, t + 0.45);

    osc1.connect(gain);
    osc2.connect(gain);
    gain.connect(this.seGain);

    osc1.start(t);
    osc2.start(t);
    osc1.stop(t + 0.46);
    osc2.stop(t + 0.46);

    // 4連鎖以上ならファンファーレアルペジオを追加発音
    if (chainCount >= 4) {
      const arpFreqs = [freq * 1.25, freq * 1.5, freq * 2];
      arpFreqs.forEach((f, i) => {
        const arpOsc = this.ctx.createOscillator();
        const arpGain = this.ctx.createGain();
        const start = t + (i + 1) * 0.06;

        arpOsc.type = 'triangle';
        arpOsc.frequency.setValueAtTime(f, start);

        arpGain.gain.setValueAtTime(0.25, start);
        arpGain.gain.exponentialRampToValueAtTime(0.001, start + 0.25);

        arpOsc.connect(arpGain);
        arpGain.connect(this.seGain);

        arpOsc.start(start);
        arpOsc.stop(start + 0.26);
      });
    }
  }

  /** おじゃまぷよ落下音（ドガガガッ） */
  playOjamaDrop() {
    this.resume();
    if (!this.ctx || this.isMuted) return;

    const t = this.ctx.currentTime;

    // ノイズバースト
    if (this.noiseBuffer) {
      const noise = this.ctx.createBufferSource();
      noise.buffer = this.noiseBuffer;

      const filter = this.ctx.createBiquadFilter();
      filter.type = 'lowpass';
      filter.frequency.setValueAtTime(800, t);
      filter.frequency.exponentialRampToValueAtTime(150, t + 0.3);

      const gain = this.ctx.createGain();
      gain.gain.setValueAtTime(0.4, t);
      gain.gain.exponentialRampToValueAtTime(0.001, t + 0.3);

      noise.connect(filter);
      filter.connect(gain);
      gain.connect(this.seGain);

      noise.start(t);
      noise.stop(t + 0.31);
    }

    // 重低音インパクト
    const bass = this.ctx.createOscillator();
    const bassGain = this.ctx.createGain();
    bass.type = 'triangle';
    bass.frequency.setValueAtTime(160, t);
    bass.frequency.exponentialRampToValueAtTime(45, t + 0.25);

    bassGain.gain.setValueAtTime(0.4, t);
    bassGain.gain.exponentialRampToValueAtTime(0.001, t + 0.25);

    bass.connect(bassGain);
    bassGain.connect(this.seGain);

    bass.start(t);
    bass.stop(t + 0.26);
  }

  /** 相殺（そうさい）SE（シャキーン！） */
  playSousai() {
    this.resume();
    if (!this.ctx || this.isMuted) return;

    const t = this.ctx.currentTime;
    const osc = this.ctx.createOscillator();
    const gain = this.ctx.createGain();

    osc.type = 'sawtooth';
    osc.frequency.setValueAtTime(880, t);
    osc.frequency.exponentialRampToValueAtTime(2200, t + 0.18);

    gain.gain.setValueAtTime(0.28, t);
    gain.gain.exponentialRampToValueAtTime(0.001, t + 0.25);

    const filter = this.ctx.createBiquadFilter();
    filter.type = 'highpass';
    filter.frequency.setValueAtTime(1200, t);

    osc.connect(filter);
    filter.connect(gain);
    gain.connect(this.seGain);

    osc.start(t);
    osc.stop(t + 0.26);
  }

  /** 危険警告音（ピンポンピンポン） */
  playDangerAlert() {
    this.resume();
    if (!this.ctx || this.isMuted) return;

    const t = this.ctx.currentTime;
    const osc = this.ctx.createOscillator();
    const gain = this.ctx.createGain();

    osc.type = 'sine';
    osc.frequency.setValueAtTime(987.77, t); // B5
    osc.frequency.setValueAtTime(783.99, t + 0.08); // G5

    gain.gain.setValueAtTime(0.25, t);
    gain.gain.exponentialRampToValueAtTime(0.001, t + 0.18);

    osc.connect(gain);
    gain.connect(this.seGain);

    osc.start(t);
    osc.stop(t + 0.19);
  }

  /** ステージクリア / 勝利ファンファーレ */
  playVictory() {
    this.resume();
    if (!this.ctx || this.isMuted) return;

    const notes = [
      { f: 523.25, d: 0.15 }, // C5
      { f: 659.25, d: 0.15 }, // E5
      { f: 783.99, d: 0.15 }, // G5
      { f: 1046.50, d: 0.45 } // C6
    ];

    let t = this.ctx.currentTime;
    notes.forEach(note => {
      const osc = this.ctx.createOscillator();
      const gain = this.ctx.createGain();

      osc.type = 'triangle';
      osc.frequency.setValueAtTime(note.f, t);

      gain.gain.setValueAtTime(0.4, t);
      gain.gain.exponentialRampToValueAtTime(0.001, t + note.d);

      osc.connect(gain);
      gain.connect(this.seGain);

      osc.start(t);
      osc.stop(t + note.d + 0.02);
      t += note.d;
    });
  }

  /** 敗北ジングル */
  playDefeat() {
    this.resume();
    if (!this.ctx || this.isMuted) return;

    const notes = [
      { f: 392.00, d: 0.2 }, // G4
      { f: 369.99, d: 0.2 }, // F#4
      { f: 349.23, d: 0.2 }, // F4
      { f: 329.63, d: 0.6 }  // E4
    ];

    let t = this.ctx.currentTime;
    notes.forEach(note => {
      const osc = this.ctx.createOscillator();
      const gain = this.ctx.createGain();

      osc.type = 'sawtooth';
      osc.frequency.setValueAtTime(note.f, t);

      gain.gain.setValueAtTime(0.3, t);
      gain.gain.exponentialRampToValueAtTime(0.001, t + note.d);

      osc.connect(gain);
      gain.connect(this.seGain);

      osc.start(t);
      osc.stop(t + note.d + 0.02);
      t += note.d;
    });
  }

  // =========================================================================
  // BGM シーケンサーエンジン
  // =========================================================================

  /**
   * BGM開始
   * @param {number} stage 1〜5
   */
  startBGM(stage = 1) {
    this.resume();
    this.currentStage = stage;
    // ステージが上がるごとにテンポ上昇 (132BPM -> 168BPM)
    this.bgmBpm = 132 + (stage - 1) * 9;
    
    if (this.isPlayingBGM) return;
    this.isPlayingBGM = true;
    this.currentStep = 0;
    this.nextNoteTime = this.ctx ? this.ctx.currentTime + 0.05 : 0;

    this.scheduleLoop();
  }

  /** BGM停止 */
  stopBGM() {
    this.isPlayingBGM = false;
    if (this.bgmTimerId) {
      clearTimeout(this.bgmTimerId);
      this.bgmTimerId = null;
    }
  }

  /** テンポ更新 */
  setStage(stage) {
    this.currentStage = stage;
    this.bgmBpm = 132 + (stage - 1) * 9;
  }

  /** ルックアヘッド スケジューラーループ */
  scheduleLoop() {
    if (!this.isPlayingBGM || !this.ctx) return;

    const secondsPerStep = 60.0 / this.bgmBpm / 4; // 16分音符
    const scheduleAheadTime = 0.15; // 150ms先まで予約

    while (this.nextNoteTime < this.ctx.currentTime + scheduleAheadTime) {
      this.scheduleStep(this.currentStep, this.nextNoteTime, secondsPerStep);
      this.nextNoteTime += secondsPerStep;
      this.currentStep = (this.currentStep + 1) % 64; // 4小節ループ (16 * 4)
    }

    this.bgmTimerId = setTimeout(() => this.scheduleLoop(), 50);
  }

  /**
   * 1ステップ（16分音符）分の発音予約
   */
  scheduleStep(step, time, stepLen) {
    if (this.isMuted) return;

    const bar = Math.floor(step / 16);
    const pos = step % 16;

    // 王道進行コード（F -> G -> Em -> Am）
    const chordRoots = [
      174.61, // Bar 0: F3
      196.00, // Bar 1: G3
      164.81, // Bar 2: E3
      220.00  // Bar 3: A3
    ];
    const root = chordRoots[bar];

    // 1. ベースライン (8分音符で刻むベース)
    if (pos % 2 === 0) {
      const bassOsc = this.ctx.createOscillator();
      const bassGain = this.ctx.createGain();

      bassOsc.type = 'triangle';
      // オクターブ変化でノリを出す
      const freq = (pos === 2 || pos === 6 || pos === 10 || pos === 14) ? root * 2 : root;
      bassOsc.frequency.setValueAtTime(freq, time);

      bassGain.gain.setValueAtTime(0.28, time);
      bassGain.gain.exponentialRampToValueAtTime(0.001, time + stepLen * 1.8);

      bassOsc.connect(bassGain);
      bassGain.connect(this.bgmGain);

      bassOsc.start(time);
      bassOsc.stop(time + stepLen * 1.9);
    }

    // 2. ドラム（スネア・キック・ハイハット）
    // キック: 0, 8
    if (pos === 0 || pos === 8) {
      const kickOsc = this.ctx.createOscillator();
      const kickGain = this.ctx.createGain();
      kickOsc.type = 'sine';
      kickOsc.frequency.setValueAtTime(140, time);
      kickOsc.frequency.exponentialRampToValueAtTime(35, time + 0.12);

      kickGain.gain.setValueAtTime(0.35, time);
      kickGain.gain.exponentialRampToValueAtTime(0.001, time + 0.12);

      kickOsc.connect(kickGain);
      kickGain.connect(this.bgmGain);

      kickOsc.start(time);
      kickOsc.stop(time + 0.13);
    }

    // スネア: 4, 12
    if ((pos === 4 || pos === 12) && this.noiseBuffer) {
      const snare = this.ctx.createBufferSource();
      snare.buffer = this.noiseBuffer;

      const filter = this.ctx.createBiquadFilter();
      filter.type = 'bandpass';
      filter.frequency.setValueAtTime(1500, time);

      const snareGain = this.ctx.createGain();
      snareGain.gain.setValueAtTime(0.25, time);
      snareGain.gain.exponentialRampToValueAtTime(0.001, time + 0.14);

      snare.connect(filter);
      filter.connect(snareGain);
      snareGain.connect(this.bgmGain);

      snare.start(time);
      snare.stop(time + 0.15);
    }

    // ハイハット: 2, 6, 10, 14
    if (pos % 2 === 0 && pos % 4 !== 0 && this.noiseBuffer) {
      const hat = this.ctx.createBufferSource();
      hat.buffer = this.noiseBuffer;

      const hatFilter = this.ctx.createBiquadFilter();
      hatFilter.type = 'highpass';
      hatFilter.frequency.setValueAtTime(6000, time);

      const hatGain = this.ctx.createGain();
      hatGain.gain.setValueAtTime(0.12, time);
      hatGain.gain.exponentialRampToValueAtTime(0.001, time + 0.04);

      hat.connect(hatFilter);
      hatFilter.connect(hatGain);
      hatGain.connect(this.bgmGain);

      hat.start(time);
      hat.stop(time + 0.05);
    }

    // 3. チップチューン リードメロディ (Square wave)
    // キャッチーなアルペジオ/スケール旋律
    const melodyNotes = [
      // Bar 0 (F)
      523.25, 659.25, 523.25, 698.46, 783.99, 698.46, 659.25, 523.25,
      659.25, 523.25, 698.46, 783.99, 880.00, 783.99, 698.46, 659.25,
      // Bar 1 (G)
      587.33, 783.99, 587.33, 783.99, 880.00, 783.99, 698.46, 587.33,
      783.99, 880.00, 987.77, 880.00, 783.99, 698.46, 587.33, 659.25,
      // Bar 2 (Em)
      659.25, 783.99, 987.77, 783.99, 659.25, 783.99, 987.77, 1046.50,
      987.77, 783.99, 659.25, 783.99, 987.77, 783.99, 659.25, 587.33,
      // Bar 3 (Am)
      523.25, 659.25, 880.00, 659.25, 523.25, 659.25, 880.00, 1046.50,
      1174.66, 1046.50, 880.00, 783.99, 659.25, 587.33, 523.25, 493.88
    ];

    const noteFreq = melodyNotes[step];
    if (noteFreq) {
      const leadOsc = this.ctx.createOscillator();
      const leadGain = this.ctx.createGain();

      leadOsc.type = 'square';
      leadOsc.frequency.setValueAtTime(noteFreq, time);

      // チップチューンらしい短いスタッカート・エンベロープ
      leadGain.gain.setValueAtTime(0.13, time);
      leadGain.gain.exponentialRampToValueAtTime(0.001, time + stepLen * 0.9);

      leadOsc.connect(leadGain);
      leadGain.connect(this.bgmGain);

      leadOsc.start(time);
      leadOsc.stop(time + stepLen * 0.95);
    }
  }
}

// シングルトンインスタンス
const soundEngine = new SoundEngine();
