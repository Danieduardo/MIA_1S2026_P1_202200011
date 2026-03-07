import { useState, useRef } from "react"
import axios from "axios"
import "./App.css"

const API = "http://localhost:3001"

export default function App() {
  const [input,   setInput]   = useState("")
  const [output,  setOutput]  = useState("# Los resultados aparecerán aquí...\n")
  const [loading, setLoading] = useState(false)
  const fileRef = useRef(null)

  // Ejecutar comandos
  const handleExecute = async () => {
    if(!input.trim()) return
    setLoading(true)
    try {
      const res = await axios.post(`${API}/execute`, { commands: input })
      setOutput(prev => prev + res.data.output)
    } catch(e) {
      setOutput(prev => prev + "Error de conexión con el servidor\n")
    }
    setLoading(false)
  }

  // Cargar archivo .smia
  const handleFile = (e) => {
    const file = e.target.files[0]
    if(!file) return
    const reader = new FileReader()
    reader.onload = (ev) => setInput(ev.target.result)
    reader.readAsText(file)
  }

  const handleClear = () => {
    setOutput("# Los resultados aparecerán aquí...\n")
  }

  return (
    <div className="app">
      <header className="header">
        <h1>ExtreamFS</h1>
        <span className="subtitle">Simulador EXT2 | Carnet: 20220011</span>
      </header>

      <div className="container">
        {/* Panel izquierdo - Entrada */}
        <div className="panel">
          <div className="panel-header">
            <span>Entrada de Comandos</span>
            <div className="btn-group">
              <button
                className="btn btn-secondary"
                onClick={() => fileRef.current.click()}>
                Cargar Archivo
              </button>
              <input
                ref={fileRef}
                type="file"
                accept=".smia"
                style={{display:"none"}}
                onChange={handleFile}
              />
              <button
                className="btn btn-primary"
                onClick={handleExecute}
                disabled={loading}>
                {loading ? "Ejecutando..." : "Ejecutar"}
              </button>
            </div>
          </div>
          <textarea
            className="terminal-input"
            value={input}
            onChange={e => setInput(e.target.value)}
            placeholder="Ingrese comandos aquí...&#10;Ejemplo:&#10;mkdisk -size=10 -unit=M -path=/home/disco1.mia"
            spellCheck={false}
          />
        </div>

        {/* Panel derecho - Salida */}
        <div className="panel">
          <div className="panel-header">
            <span>Salida</span>
            <button className="btn btn-secondary" onClick={handleClear}>
              Limpiar
            </button>
          </div>
          <pre className="terminal-output">{output}</pre>
        </div>
      </div>
    </div>
  )
}