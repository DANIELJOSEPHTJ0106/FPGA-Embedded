import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox
import serial
import serial.tools.list_ports
import threading
import math

# Try to import Matplotlib for the 3D view
try:
    from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
    from matplotlib.figure import Figure
    from mpl_toolkits.mplot3d import Axes3D
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False

class UltimateGPSApp:
    def __init__(self, root):
        self.root = root
        self.root.title("u-blox EVK-X20P Mission Control (Multi-Band Edition)")
        self.root.geometry("1300x850") 
        
        # --- Serial Variables ---
        self.ser = None
        self.is_running = False
        self.selected_port = tk.StringVar()
        
        # --- Data Storage ---
        self.labels = {} 
        self.satellites = {} # For 3D Plot
        
        # --- Layout ---
        self.create_menu()
        
        # Create Tabs
        self.notebook = ttk.Notebook(self.root)
        self.notebook.pack(fill="both", expand=True, padx=10, pady=10)
        
        self.tab_dashboard = tk.Frame(self.notebook)
        self.tab_3d = tk.Frame(self.notebook)
        
        self.notebook.add(self.tab_dashboard, text="  DATA DASHBOARD  ")
        self.notebook.add(self.tab_3d, text="  3D SKYPLOT  ")
        
        self.setup_dashboard(self.tab_dashboard)
        self.setup_3d_view(self.tab_3d)
        
        self.refresh_ports()

    def create_menu(self):
        conn_frame = tk.Frame(self.root, bg="#eee", pady=10)
        conn_frame.pack(fill="x", side="top")
        
        tk.Label(conn_frame, text="Select Port:", bg="#eee").pack(side="left", padx=10)
        self.port_combo = ttk.Combobox(conn_frame, textvariable=self.selected_port, width=15)
        self.port_combo.pack(side="left")
        
        tk.Button(conn_frame, text="Refresh", command=self.refresh_ports).pack(side="left", padx=5)
        self.btn_connect = tk.Button(conn_frame, text="CONNECT", bg="black", fg="white", font=("bold", 10), command=self.toggle_connection)
        self.btn_connect.pack(side="left", padx=20)

    def setup_dashboard(self, parent):
        top_frame = tk.Frame(parent)
        top_frame.pack(fill="x", pady=5)
        
        # Col 1: Pos & Time
        col1 = tk.Frame(top_frame)
        col1.pack(side="left", fill="both", expand=True, padx=5)
        self.create_group(col1, "1. Positioning", [("Lat", "lat"), ("Lon", "lon"), ("Alt", "alt"), ("Geoid", "geoid")])
        self.create_group(col1, "2. Timing", [("Time", "time"), ("Date", "date")])

        # Col 2: Motion
        col2 = tk.Frame(top_frame)
        col2.pack(side="left", fill="both", expand=True, padx=5)
        self.create_group(col2, "3. Motion", [("Speed (kn)", "speed_kn"), ("Speed (km/h)", "speed_km"), ("Course", "course")])
        self.create_group(col2, "4. Accuracy", [("Fix", "fix"), ("Sats", "sats_used"), ("PDOP", "pdop"), ("HDOP", "hdop")])

        # Col 3: Satellite Table (With Frequency Column)
        col3 = tk.Frame(top_frame)
        col3.pack(side="left", fill="both", expand=True, padx=5)
        tk.Label(col3, text="5. Satellite Signals", font=("Arial", 10, "bold")).pack(anchor="w")
        
        cols = ("ID", "Sys", "Freq", "Elv", "Az", "SNR")
        self.sat_tree = ttk.Treeview(col3, columns=cols, show='headings', height=10)
        
        self.sat_tree.heading("ID", text="ID")
        self.sat_tree.column("ID", width=35, anchor="center")
        
        self.sat_tree.heading("Sys", text="Sys")
        self.sat_tree.column("Sys", width=35, anchor="center")
        
        self.sat_tree.heading("Freq", text="Freq (MHz)") 
        self.sat_tree.column("Freq", width=90, anchor="center")
        
        self.sat_tree.heading("Elv", text="Elv")
        self.sat_tree.column("Elv", width=35, anchor="center")
        
        self.sat_tree.heading("Az", text="Az")
        self.sat_tree.column("Az", width=35, anchor="center")
        
        self.sat_tree.heading("SNR", text="SNR")
        self.sat_tree.column("SNR", width=35, anchor="center")
        
        self.sat_tree.pack(fill="both", expand=True)

        # Bottom Section
        bot_frame = tk.Frame(parent)
        bot_frame.pack(fill="both", expand=True, pady=10)

        t_frame = tk.LabelFrame(bot_frame, text="Date & Time Log (IST)", padx=5, pady=5)
        t_frame.pack(side="left", fill="both", expand=True, padx=(0, 5))
        self.txt_time = scrolledtext.ScrolledText(t_frame, height=10, state='disabled', font=("Consolas", 10, "bold"), bg="black", fg="#00ff00")
        self.txt_time.pack(fill="both", expand=True)

        r_frame = tk.LabelFrame(bot_frame, text="Raw NMEA Stream", padx=5, pady=5)
        r_frame.pack(side="left", fill="both", expand=True)
        self.txt_raw = scrolledtext.ScrolledText(r_frame, height=10, state='disabled', font=("Consolas", 9), bg="#1e1e1e", fg="white")
        self.txt_raw.pack(fill="both", expand=True)

    def setup_3d_view(self, parent):
        if not HAS_MATPLOTLIB: return
        self.fig = Figure(figsize=(5, 5), dpi=100)
        self.ax = self.fig.add_subplot(111, projection='3d')
        self.ax.set_title("Live Satellite Positions")
        self.canvas = FigureCanvasTkAgg(self.fig, master=parent)
        self.canvas.get_tk_widget().pack(fill="both", expand=True)
        self.animate_3d()

    def create_group(self, parent, title, fields):
        frame = tk.LabelFrame(parent, text=title, font=("Arial", 11, "bold"), fg="blue")
        frame.pack(fill="x", pady=2)
        for label, key in fields:
            row = tk.Frame(frame)
            row.pack(fill="x")
            tk.Label(row, text=label+":", width=12, anchor="e", fg="#555").pack(side="left")
            val_lbl = tk.Label(row, text="--", font=("Arial", 10, "bold"), anchor="w", fg="black")
            val_lbl.pack(side="left", padx=5)
            self.labels[key] = val_lbl

    def refresh_ports(self):
        ports = [p.device for p in serial.tools.list_ports.comports()]
        self.port_combo['values'] = ports
        if ports: self.port_combo.current(0)

    def toggle_connection(self):
        if not self.is_running:
            try:
                self.ser = serial.Serial(self.selected_port.get(), 38400, timeout=1)
                self.is_running = True
                self.btn_connect.config(text="DISCONNECT", bg="red")
                threading.Thread(target=self.read_serial, daemon=True).start()
            except Exception as e:
                messagebox.showerror("Connection Error", str(e))
        else:
            self.is_running = False
            if self.ser: self.ser.close()
            self.btn_connect.config(text="CONNECT", bg="black")

    def read_serial(self):
        while self.is_running and self.ser:
            try:
                line = self.ser.readline().decode('utf-8', errors='replace').strip()
                if line:
                    self.parse_nmea(line)
                    self.log_raw(line)
            except:
                break
    
    # --- THE MASTER FREQUENCY DECODER ---
    def get_frequency_value(self, sys_id, sig_id):
        # 1. Handle "Searching" or Empty
        if not sig_id or sig_id == '0': 
            return "Searching..."
        
        # 2. GPS (USA)
        if sys_id == "GP":
            if sig_id == '1': return "1575.42 MHz" # L1 C/A
            if sig_id == '5': return "1227.60 MHz" # L2 CM
            if sig_id == '6': return "1227.60 MHz" # L2 CL
            if sig_id == '8': return "1176.45 MHz" # L5 (Modern)
            
        # 3. Galileo (Europe)
        elif sys_id == "GA":
            if sig_id == '7': return "1575.42 MHz" # E1
            if sig_id == '1': return "1575.42 MHz" # E1 (Old)
            if sig_id == '2': return "1176.45 MHz" # E5a
            if sig_id == '8': return "1207.14 MHz" # E5b
            if sig_id == '3': return "1278.75 MHz" # E6 (HAS)
            
        # 4. BeiDou (China)
        elif sys_id == "GB":
            if sig_id == '1': return "1561.09 MHz" # B1I
            if sig_id == '3': return "1575.42 MHz" # B1C
            if sig_id == '5': return "1176.45 MHz" # B2a (Precision)
            if sig_id == '7': return "1207.14 MHz" # B2b
            if sig_id == '8': return "1268.52 MHz" # B3I (High Reliability)
            
        # 5. GLONASS (Russia)
        elif sys_id == "GL":
            if sig_id == '1': return "1602.00 MHz" # L1OF
            if sig_id == '3': return "1246.00 MHz" # L2OF

        # 6. NavIC (India)
        elif sys_id == "GI":
            if sig_id == '1': return "1176.45 MHz" # L5

        # 7. QZSS (Japan)
        elif sys_id == "GQ":
            if sig_id == '1': return "1575.42 MHz" # L1
            if sig_id == '8': return "1176.45 MHz" # L5

        return f"Sig {sig_id}" # Fallback

    def parse_nmea(self, line):
        parts = line.split(',')
        if not parts: return

        # 1. RMC (Time, Date, Pos)
        if "RMC" in parts[0]:
            valid = (len(parts)>2 and parts[2]=='A')
            
            # Time Log (IST Conversion)
            time_str, date_str = "--:--:--", "--/--/--"
            if len(parts)>1 and parts[1]: 
                t = parts[1]
                try:
                    h = int(t[0:2])
                    m = int(t[2:4])
                    s = int(t[4:6])
                    # Add 5 hours 30 mins
                    m += 30
                    if m >= 60:
                        m -= 60
                        h += 1
                    h += 5
                    if h >= 24:
                        h -= 24
                    time_str = f"{h:02}:{m:02}:{s:02}"
                    self.update_lbl("time", time_str + " IST", True)
                except: pass

            if len(parts)>9 and parts[9]:
                d = parts[9]
                date_str = f"{d[0:2]}/{d[2:4]}/20{d[4:6]}"
                self.update_lbl("date", date_str, True)
            
            if time_str != "--:--:--":
                self.log_time(f"Time: {time_str} IST | Date: {date_str}")

            if valid:
                self.update_lbl("lat", f"{parts[3]} {parts[4]}", True)
                self.update_lbl("lon", f"{parts[5]} {parts[6]}", True)
                self.update_lbl("speed_kn", parts[7], True)
                self.update_lbl("course", parts[8], True)
            else:
                self.update_lbl("lat", "No Fix", False)

        # 2. VTG
        if "VTG" in parts[0] and len(parts)>7:
            self.update_lbl("speed_km", parts[7] + " km/h", True)

        # 3. GGA
        if "GGA" in parts[0]:
            qual = parts[6] if len(parts)>6 else '0'
            is_fix = (qual != '0')
            self.update_lbl("fix", f"Mode {qual}", is_fix)
            self.update_lbl("sats_used", parts[7], is_fix)
            self.update_lbl("hdop", parts[8], is_fix)
            self.update_lbl("alt", f"{parts[9]} M", is_fix)
            self.update_lbl("geoid", f"{parts[11]} M", is_fix)

        # 4. GSV (Satellites)
        if "GSV" in parts[0]:
            try:
                sys_id = parts[0][1:3]
                # Extract Signal ID (Last field before checksum)
                raw_sig_id = parts[-1].split('*')[0]
                
                # Get the frequency string (e.g., "1176.45 MHz")
                freq_val = self.get_frequency_value(sys_id, raw_sig_id)

                sats_list = []
                for i in range(4, len(parts)-1, 4):
                    if i+3 < len(parts):
                        sid = parts[i]
                        elv = parts[i+1]
                        az = parts[i+2]
                        snr = parts[i+3].split('*')[0]
                        if sid and snr:
                            # Add data to table
                            sats_list.append((sid, sys_id, freq_val, elv, az, snr))
                            
                            # Add data to 3D plot
                            if elv and az:
                                self.satellites[sid] = (float(az), float(elv), int(snr))
                                
                self.root.after(0, self.update_tree, sats_list)
            except: pass

    # --- GUI UPDATES ---
    def update_lbl(self, key, val, status):
        self.root.after(0, lambda: self._safe_config(key, val, status))

    def _safe_config(self, key, val, status):
        if key in self.labels:
            color = "#00aa00" if status else "black"
            self.labels[key].config(text=val, fg=color)

    def log_raw(self, text):
        self.root.after(0, lambda: self._append_text(self.txt_raw, text))

    def log_time(self, text):
        self.root.after(0, lambda: self._append_text(self.txt_time, text))

    def _append_text(self, widget, text):
        widget.config(state='normal')
        widget.insert(tk.END, text + "\n")
        widget.see(tk.END)
        widget.config(state='disabled')

    def update_tree(self, data):
        for item in data:
            self.sat_tree.insert("", 0, values=item)
        children = self.sat_tree.get_children()
        # Keep table clean (max 30 rows)
        if len(children) > 30:
            for c in children[30:]: self.sat_tree.delete(c)

    # --- 3D ANIMATION ---
    def animate_3d(self):
        if not HAS_MATPLOTLIB: return
        self.ax.clear()
        
        # Draw Dome
        import numpy as np
        u = np.linspace(0, 2 * np.pi, 30)
        v = np.linspace(0, np.pi / 2, 10)
        x_wire = np.outer(np.cos(u), np.sin(v))
        y_wire = np.outer(np.sin(u), np.sin(v))
        z_wire = np.outer(np.ones(np.size(u)), np.cos(v))
        self.ax.plot_wireframe(x_wire, y_wire, z_wire, color='gray', alpha=0.1)
        
        # Compass Labels
        self.ax.text(0, 1.1, 0, "N", color='blue')
        self.ax.text(0, -1.1, 0, "S", color='blue')
        self.ax.text(1.1, 0, 0, "E", color='blue')
        self.ax.text(-1.1, 0, 0, "W", color='blue')

        # Plot Satellites
        for sid, (az, el, snr) in list(self.satellites.items()):
            az_rad = math.radians(az)
            el_rad = math.radians(el)
            
            # Convert Spherical to Cartesian
            x = math.cos(el_rad) * math.sin(az_rad)
            y = math.cos(el_rad) * math.cos(az_rad)
            z = math.sin(el_rad)
            
            # Color Code
            color = 'red'
            if snr > 20: color = 'orange'
            if snr > 30: color = '#00ff00' # Green
            
            self.ax.scatter(x, y, z, color=color, s=snr*2)
            self.ax.plot([x, x], [y, y], [0, z], color='gray', linestyle=':', alpha=0.5)
            self.ax.text(x, y, z, sid, fontsize=8)

        self.ax.set_axis_off()
        self.canvas.draw()
        self.root.after(1000, self.animate_3d)

if __name__ == "__main__":
    root = tk.Tk()
    app = UltimateGPSApp(root)
    root.mainloop()
