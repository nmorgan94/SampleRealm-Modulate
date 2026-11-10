// The engine renders this many at most per call
const MAX_BLOCK = 128;
const LEVEL_EVERY_BLOCKS = 8;
// About 30 times a second
const LIVE_EVERY_BLOCKS = 12;
const SOURCES = 4;

class ModulateProcessor extends AudioWorkletProcessor {
  constructor({ processorOptions: { wasm } }) {
    super();

    try {
      // WASI imports are never called
      const stub = new Proxy({}, { get: () => () => 0 });
      const instance = new WebAssembly.Instance(new WebAssembly.Module(wasm), new Proxy({}, { get: () => stub }));
      this.engine = instance.exports;
      this.engine._initialize?.();
      this.engine.prepare(sampleRate);
    } catch (error) {
      this.engine = null;
      this.port.postMessage({ type: 'error', message: String(error) });
      return;
    }

    this.blocks = 0;
    this.lastSilent = false;

    this.port.onmessage = ({ data }) => {
      const engine = this.engine;
      if (data.type === 'param') engine.setParam(data.index, data.value);
      else if (data.type === 'modulation') engine.setModulation(data.index, data.source, data.sourceIndex, data.min, data.max);
      else if (data.type === 'transport') engine.setTransport(data.bpm, data.playing, data.ppq);
      else if (data.type === 'midi') engine.midi(data.bytes[0], data.bytes[1], data.bytes[2]);
      else if (data.type === 'curve') {
        const count = Math.min(data.points.length / 2, engine.maxCurvePoints());
        this.floats(engine.points(), 2 * count).set(data.points.slice(0, 2 * count));
        engine.setCurve(data.lfo, count);
      }
    };

    this.port.postMessage({ type: 'ready' });
  }

  floats(pointer, length) {
    return new Float32Array(this.engine.memory.buffer, pointer, length);
  }

  // Views go stale if memory grows
  channels() {
    if (this.channelsBuffer !== this.engine.memory.buffer) {
      this.channelsBuffer = this.engine.memory.buffer;
      this.channelViews = [0, 1].map((index) => this.floats(this.engine.channel(index), MAX_BLOCK));
    }
    return this.channelViews;
  }

  process(_inputs, [output]) {
    if (!this.engine) return false;

    const frames = output[0].length;

    for (let start = 0; start < frames; start += MAX_BLOCK) {
      const count = Math.min(MAX_BLOCK, frames - start);
      this.engine.render(count);
      const views = this.channels();

      for (let index = 0; index < output.length; index++) {
        const view = views[Math.min(index, 1)];
        const channel = output[index];
        for (let i = 0; i < count; i++) channel[start + i] = view[i];
      }
    }

    this.blocks++;

    if (this.blocks % LEVEL_EVERY_BLOCKS === 0) {
      const peaks = [this.engine.peak(0), this.engine.peak(1)];
      const silent = peaks.every((peak) => peak === 0);
      if (!silent || !this.lastSilent) this.port.postMessage({ type: 'level', peaks });
      this.lastSilent = silent;
    }

    if (this.blocks % LIVE_EVERY_BLOCKS === 0) {
      const values = Array.from(this.floats(this.engine.live(), 4 * SOURCES));
      const live = {
        lfoPhase: values.slice(0, SOURCES),
        lfoValue: values.slice(SOURCES, 2 * SOURCES),
        envelopeLevel: values.slice(2 * SOURCES, 3 * SOURCES),
        envelopeProgress: values.slice(3 * SOURCES),
        envelopeStage: Array.from({ length: SOURCES }, (_, i) => this.engine.envelopeStage(i)),
      };
      this.port.postMessage({ type: 'live', live });
    }

    return true;
  }
}

registerProcessor('modulate-processor', ModulateProcessor);
