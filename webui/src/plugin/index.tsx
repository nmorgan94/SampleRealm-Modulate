import { createRoot } from 'react-dom/client';
import { pluginBackend } from './pluginBackend.ts';
import { ModulateEditor } from '../ModulateEditor.tsx';
import './styles.css';

const root = createRoot(document.getElementById('root')!);

try {
  root.render(<ModulateEditor backend={pluginBackend()} />);
} catch (error) {
  console.error(error);
  root.render(<pre className="p-5 text-fg">{`The editor couldn't connect to the plugin.\n${error}`}</pre>);
}
