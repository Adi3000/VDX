﻿using System.ServiceModel;
using System.ServiceProcess;
using HidCerberus.Lib;


namespace HidCerberus.Srv
{
    public partial class HidCerberusService : ServiceBase
    {
        private static ServiceHost _serviceHost;

        public HidCerberusService()
        {
            InitializeComponent();
        }

        protected override void OnStart(string[] args)
        {
            _serviceHost?.Close();

            var binding = new NetTcpBinding
            {
                TransferMode = TransferMode.Streamed,
                Security = new NetTcpSecurity { Mode = SecurityMode.None }
            };

            _serviceHost = new ServiceHost(typeof(HidCerberusWcf), Lib.HidCerberus.WcfUrl);
            _serviceHost.AddServiceEndpoint(typeof (IHidCerberusWcf), binding, Lib.HidCerberus.WcfUrl);

            _serviceHost.Open();
        }

        protected override void OnStop()
        {
        }
    }
}
