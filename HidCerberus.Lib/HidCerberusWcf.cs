﻿using System;
using System.Reflection;
using log4net;
using Microsoft.Win32;

namespace HidCerberus.Lib
{
    public class HidCerberusWcf : IHidCerberusWcf
    {
        private static readonly ILog Log = LogManager.GetLogger(MethodBase.GetCurrentMethod().DeclaringType);

        public void AddPid(int id)
        {
            Log.Info($"Adding PID {id}");
            try
            {
                Registry.LocalMachine.CreateSubKey($"{HidCerberus.WhitelistRegistryKeyBase}\\{id}");
            }
            catch (Exception ex)
            {
                Log.Error(ex);
            }
        }

        public void RemovePid(int id)
        {
            Log.Info($"Removing PID {id}");
            try
            {
                Registry.LocalMachine.DeleteSubKey($"{HidCerberus.WhitelistRegistryKeyBase}\\{id}");
            }
            catch (Exception ex)
            {
                Log.Error(ex);
            }
        }
    }
}