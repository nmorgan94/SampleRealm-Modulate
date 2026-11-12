// The plugin's own factory presets
const files = import.meta.glob<string>('../../../Assets/Presets/*.modulate', { query: '?raw', import: 'default', eager: true });

export const factoryPresets = Object.values(files);
