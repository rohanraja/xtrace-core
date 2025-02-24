'use client'

import { useState, useEffect } from "react";
import { Button } from "@/components/ui/button";
import { getConfigs, getLastLogContents, saveConfig } from "./actions/getConfigs";
import { runE2E } from "./actions/runActions";
import { Card, CardContent } from "@/components/ui/card";
import { Textarea } from "@/components/ui/textarea";
import { Select, SelectTrigger, SelectValue, SelectContent, SelectItem } from "@/components/ui/select";

export default function ConfigEditor() {
  const [configs, setConfigs] = useState<{ name: string; json: string; logs: string[] }[]>([]);
  const [selectedConfig, setSelectedConfig] = useState<string | null>(null);
  const [jsonInput, setJsonInput] = useState<string>("{}");
  const [logs, setLogs] = useState<string[]>([]);

  const handleRun = () => {
    setLogs((prevLogs) => [
      `Running pipeline with config: ${jsonInput}`,
      ...prevLogs,
    ]);

    runE2E(selectedConfig!);
  };
  const handleSave = () => {
    setLogs((prevLogs) => [
      `Saving changes`,
      ...prevLogs,
    ]);

    saveConfig(selectedConfig!, jsonInput);
  };

  const handleConfigChange = (name: string) => {
    const config = configs.find((c) => c.name === name);
    if (config) {
      setSelectedConfig(name);
      setJsonInput(config.json);
      setLogs(config.logs);

      async function fetchData(){
        const lastLogs = await getLastLogContents(config!.name);
        // For each line, create a new line in the logs
        const logsArr = lastLogs.split("\n");
        setLogs(logsArr);
        // setLogs([lastLogs]);
      }
      setInterval(()=>{
        fetchData();
      }, 1000);
    }


  };

  const handleNewConfig = () => {
    const name = prompt("Enter config name:");
    if (name && !configs.some((c) => c.name === name)) {
      const newConfig = { name, json: "{}", logs: [] };
      setConfigs([...configs, newConfig]);
      setSelectedConfig(name);
      setJsonInput("{}");
      setLogs([]);
    }
  };

  // On pageload get configs
  useEffect(()=>{
    async function fetchData(){
        const configs = await getConfigs();
        setConfigs(configs);
    }
    fetchData();
  }, []);

  return (
    <div className="p-6 space-y-4">
      <div className="flex items-center gap-4">
        <Select onValueChange={handleConfigChange}>
          <SelectTrigger className="w-[200px]">
            <SelectValue placeholder="Select Config" />
          </SelectTrigger>
          <SelectContent>
            {configs.map((config) => (
              <SelectItem key={config.name} value={config.name}>
                {config.name}
              </SelectItem>
            ))}
          </SelectContent>
        </Select>
        <Button onClick={handleNewConfig}>New Config</Button>
      </div>
      <Textarea
        className="w-full h-40 p-2 border rounded"
        value={jsonInput}
        onChange={(e) => setJsonInput(e.target.value)}
      />
      <Button onClick={handleRun}>Run</Button>
      <Button onClick={handleSave}>Save</Button>
      <Card>
        <CardContent className="p-4 overflow-auto">
          {logs.map((log, index) => (
            <p key={index} className="text-sm text-gray-600">
              {log}
            </p>
          ))}
        </CardContent>
      </Card>
    </div>
  );
}

